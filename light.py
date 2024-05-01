import PySimpleGUI as sg
import serial as ser
import time
import math

Str_distance = '50'
angle_counter = 0
N_ADC = 204.6  # 1023 * 0.2 --> Vcc is 5 Volt
number_of_scan = 61
break_calibration = False

enableTX = True
calibrated = False
GRAPH_SIZE = (450, 450)

objects = []
ldr_calibrated = []
temp_ldr_calibrated = []
ldr_measurement = []


def scanning(scan, scan_list, s):
    """ Reading the actual LDR scans """
    global enableTX, objects, calibrated
    angle_conter = 0
    counter = 0
    scan_popup = popup("scanning for lights...")
    scan_popup.refresh()  # used to see the window on screen without reading events from it
    print("start scan!")
    while scan:
        i = 0
        info = [0, 0]
        while s.in_waiting > 0:  # while the input buffer isn't empty
            enableTX = False
            temp = s.readline()
            tempbyte = str(temp.decode("utf-8"))
            print("RX: ", tempbyte)
            if tempbyte[0] == 'c':
                calibrated = False
                scan_popup.close()
                return
            info[i] = (int(tempbyte))
            # print(info[i])
            i = i + 1
            if i > 1:                                                 # --> plaster
                print("index i in scanning is bigger then 1!\n")      # --> debugging
                print("info: ", info, "\n")                           # --> debugging
                print("objects: ", objects, "\n")                     # --> debugging
                break
            info[i] = angle_conter
            angle_conter += int((1800 / (number_of_scan * 10)) + 0.5)
            info_temp0 = info[0]/N_ADC  # calculating the actual vlotage

            if info_temp0 >= 5:  # got 1023?
                info_temp0 = 6  # there is nothing there!
            real_info = (info_temp0, info[1])
            scan_list.append(real_info)
            counter += 1
            if counter == number_of_scan:
                print("counter: ", counter, "\n")
                scan = False
                break

    scan_popup.close()
    print("scan_list:\n", scan_list)



def calibration(scan, s):
    """ Calibration of both LDR1 and LDR2, computing average of every point in area on MCU and sends here the result """
    global enableTX, calibrated, break_calibration, ldr_calibrated, temp_ldr_calibrated
    temp_ldr_calibrated = ldr_calibrated
    ldr_calibrated.clear()
    ldr_measurement.clear()

    start_scan = popup_new_dis("                    SET DISTANCE!\n\nmeasurement will not continue until 'Ok' pressed\n"
                                           "or received 'o' from MCU (pressing push button 0).\n"
                           "                   Press Cancel to Exit")
    st_scan = True
    s.reset_input_buffer()
    while st_scan:
        eventD, valD = start_scan.read(timeout=50, timeout_key="_TIMEOUT_")
        if eventD == 'Ok':
            start_scan.close()
            break
        elif eventD == "Cancel":
            start_scan.close()
            ldr_measurement.clear()
            break_calibration = True
            if calibrated:
                return True
            ldr_calibrated = temp_ldr_calibrated
            sendchar('d', s)  # notify MCU that calibration is canceled!
            return calibrated
        elif eventD == "_TIMEOUT_":
            while s.in_waiting > 0:
                enableTX = False
                temp = s.readline()
                char = temp.decode("utf-8")
                print("start window RX: ", char)
                if char[0] == 'o':
                    # received from MCU 'o' to get a new scan. char[0] is a plaster, sometimes we get 'oo'
                    start_scan.close()
                    st_scan = False
    sendchar('r', s)  # signal the MCU to continue
    calibrate = popup("Calibrating...")
    calibrate.refresh()
    while(1):
        while s.in_waiting > 0 and scan:  # while the input buffer isn't empty
            enableTX = False
            temp = s.readline()
            char = temp.decode("utf-8")
            print(char)

            if char == 'E':  # Error in the correct scan, keep the light in the same distance and try again
                eror = True
                error_scan = popup_new_dis(
                    "        ERROR IN SCAN!        TRY AGAIN!\n\nmeasurement will not continue until 'Ok' pressed\n"
                    "or received 'o' from MCU (pressing push button 0).\n"
                           "                   Press Cancel to Exit")

                while eror:
                    eventD, valD = error_scan.read(timeout=50, timeout_key="_TIMEOUT_")
                    if eventD == "Ok":
                        error_scan.close()
                        break
                    elif eventD == "Cancel":
                        error_scan.close()
                        ldr_measurement.clear()
                        break_calibration = True
                        ldr_calibrated = temp_ldr_calibrated
                        calibrate.close()
                        sendchar('d', s)  # signal the MCU to cancel
                        if calibrated:
                            return True
                        return calibrated
                    elif eventD == "_TIMEOUT_":
                        while s.in_waiting > 0:
                            enableTX = False
                            temp = s.readline()
                            char = temp.decode("utf-8")
                            print("error window RX: ", char)
                            if char[0] == 'o':
                                # received from MCU 'o' to get a new scan. char[0] is a plaster, sometimes we get 'oo'
                                error_scan.close()
                                eror = False

                sendchar('r', s)  # signal the MCU to continue
                continue
            elif char == 'n':  # request for changing light distance
                new_line = True
                change_dis = popup_new_dis("                  CHANGE DISTANCE!\n\nmeasurement will not continue until 'Ok' pressed\n"
                                           "or received 'o' from MCU (pressing push button 0).\n"
                           "                   Press Cancel to Exit")

                while new_line:
                    eventD, valD = change_dis.read(timeout=50, timeout_key="_TIMEOUT_")
                    if eventD == "Ok":
                        change_dis.close()
                        break
                    elif eventD == "Cancel":
                        change_dis.close()
                        ldr_measurement.clear()
                        break_calibration = True
                        ldr_calibrated = temp_ldr_calibrated
                        calibrate.close()
                        sendchar('d', s)  # signal the MCU to cancel
                        if calibrated:
                            return True
                        return calibrated
                    elif eventD == "_TIMEOUT_":
                        while s.in_waiting > 0:
                            enableTX = False
                            temp = s.readline()
                            char = temp.decode("utf-8")
                            print("change distance window RX: ", char)
                            if char[0] == 'o':
                                # received from MCU 'o' to get a new scan. char[0] is a plaster, sometimes we get 'oo'
                                change_dis.close()
                                new_line = False
                sendchar('r', s)  # signal the MCU to continue
                continue
            elif char == 'd':  # calibration done
                calibrate.close()
                print("ldr_measurement:\n", ldr_measurement)  # this list now contain the ldr measurement from the calibration process
                sg.popup("Calibration finished!", auto_close=True, auto_close_duration=1, any_key_closes=True)
                for j in range(0, 10):
                    # now we are building the for ldr calibrated list for every cm from 0 to 50.
                    for i in range(0, 5):
                        if j != 0:
                            m = int(ldr_measurement[j]) - int(ldr_measurement[j - 1]) / 5
                        else:
                            m = int(ldr_measurement[j+1]) - int(ldr_measurement[j]) / 5
                        ldr_calibrated.append((i * m) + ldr_measurement[j])
                enableTX = True
                calibrated = True
                print("ldr_calibrated:\n", ldr_calibrated)
                return True
            elif char[0] == 'o':
                continue
            else:
                print("this is distance: ", len(ldr_measurement))
                ldr_measurement.append((int(char) / N_ADC))  # reading from MCU the LDR average results
                sendchar('r', s)   # signal the MCU to continue


def startSweep(char, s):
    global enableTX
    # tx to mcu to start sweep
    enableTX = True
    s.reset_output_buffer()
    s.reset_input_buffer()
    while s.out_waiting > 0 or enableTX:  # while the output buffer isn't empty
        bytetxMsg = bytes(char + '\n', 'ascii')  # 'r' for MCU is to start sweep
        s.write(bytetxMsg)
        if s.out_waiting == 0:
            enableTX = False
    #  end of sending start sweep bit


def popup(message):
    layout = [[sg.Text(message, font="any 16", text_color="red")]]
    window = sg.Window('Message', layout, no_titlebar=True, keep_on_top=True, finalize=True)
    return window


def popup_new_dis(message):
    layout = [[sg.Text(message)], [sg.T("              "), sg.B('Ok', size=(6,2)),sg.T("  "), sg.B('Cancel', size=(6,2))]]
    window = sg.Window('Message', layout, no_titlebar=True, keep_on_top=True, finalize=True)
    return window


def sendchar(char, s):
    """ sending 1 char to MCU """
    global enableTX
    enableTX = True
    s.reset_output_buffer()
    while (s.out_waiting > 0 or enableTX):
        bytetxstate = bytes(char + '\n', 'ascii')
        s.write(bytetxstate)
        if s.out_waiting == 0:
            enableTX = False


def popup_new_dis_button(message):
    layout = [[sg.Text(message), sg.B('Cancel', size=(6,2))]]
    window = sg.Window('Message', layout, no_titlebar=True, keep_on_top=True, finalize=True)
    return window


def light(com):

    global Str_distance, objects, enableTX, s, calibrated, break_calibration
    s = com
    ldr_scan = []

    x_offset = 225
    y_offset = 100
    scan = True
    printscan = True
    # tx to mcu to start sweep
    print("calibrated status: ", calibrated)
    while not calibrated:
        calibrated = calibration(scan, s)
        if break_calibration:
            break_calibration = False
            return
    if calibrated:
        startSweep('r', s)   # signal the MCU to wake up
        scanning(scan, ldr_scan, s)
        while not calibrated:  # if received 'c' from push button during the scan
            calibrated = calibration(scan, s)
            if break_calibration:
                break_calibration = False
                return
            scanning(scan, ldr_scan, s)
        scan = False
    #  end of sending start sweep bit

    # Define the layout
    layout = [
        [sg.T("         Light Detector", font="any 30 bold", text_color='red', size=(0, 1))],
        [sg.T("              System", font="any 30 bold", text_color='red')],
        [sg.T(" ", size=(1, 2))],
        [sg.B('calibration', button_color=('dark blue', 'white'), pad=(180, 0), size=(10, 2))],
        [sg.T(" ", size=(1, 2))],
        [sg.Graph(canvas_size=GRAPH_SIZE, graph_top_right=GRAPH_SIZE, graph_bottom_left=(0, 0),
                  background_color='black', key="_GRAPH_")],
        [sg.B("Rescan!"), sg.B("Main Menu", pad=(130, 20))]
    ]

    window = sg.Window('Light Proximity Detector', layout, location=(500, 20))
    window.finalize()
    graph = window["_GRAPH_"]

    graph.draw_arc((10, -160), (440, 360), 180, 0, style='arc', arc_color='green')
    graph.draw_arc((60, -110), (390, 310), 180, 0, style='arc', arc_color='green')
    graph.draw_arc((110, -60), (340, 260), 180, 0, style='arc', arc_color='green')
    graph.DrawLine((10, 100), (440, 100), width=2, color="white")
    while True:
        event, values = window.read(timeout=50, timeout_key="_TIMEOUT_")
        #  worked with timeout=200 (with no push button read!), change timeout to 50 cause of calibration button

        if event == '_TIMEOUT_':
            while s.in_waiting > 0:  # for reading the push button 0 press
                enableTX = False
                temp = s.read()
                char = temp.decode("utf-8")
                print("char: ", char, "char[0]: ", char[0])
                if char[0] == 'c':
                    s.reset_input_buffer()
                    calibrated = calibration(True, s)
                    if break_calibration:

                        break_calibration = False
                        break
                    else:
                        printscan = True

            if calibrated:
                if printscan:  # print the scans
                    # establish objects list
                    print('printing on screen!')
                    for i in range(0, len(ldr_scan)):
                        voltage = ldr_scan[i][0]
                        if voltage > 5:
                            continue
                        for j in range(0, len(ldr_calibrated)):
                            if abs(ldr_calibrated[j] - voltage) < 0.0073:  # PLAY WITH MARGIN!  1.5/N_adc  -->  + - 1.5
                                objects.append((j, ldr_scan[i][1]))
                    # objects list ready
                    print("object list:\n", objects)
                    printscan = False
                    for i in range(0, len(objects)):  # putting the objects on the graph
                        distance = objects[i][0] + 1
                        angle = math.radians(objects[i][1])
                        if distance <= int(Str_distance):
                            graph.draw_text("O", location=(
                                distance * math.cos(angle) * 0.47 * 9 + x_offset, distance * math.sin(angle) * 0.58 * 9 + y_offset),
                                            # 225 is offset of x-axis and 0.47 = 215/450,     TIMES 9 TO SCALE THE 50cm TO THE END OF THE GRAPH
                                            # +100 is the height of the base line and 0.58 = 260/450 where 260 is the max of the arc
                                            font="any 14", color="light blue")
                            if (math.ceil(math.degrees(angle))) > 160:
                                text_offset = 8
                            elif (math.ceil(math.degrees(angle))) < 20:
                                text_offset = -8
                            else:
                                text_offset = 0
                            graph.draw_text(f'({int(distance)}, {math.ceil(math.degrees(angle))}°)', location=(
                                distance * math.cos(angle) * 0.47 * 9 + x_offset + text_offset,
                                distance * math.sin(angle) * 0.58 * 9 + y_offset - 15),
                                            font="any 9", color="white")
                            window["_GRAPH_"].update()

            else:
                sg.popup("need to calibrate LDRs first!!", font="any 20 bold", auto_close=True,
                         auto_close_duration=1,
                         text_color="dark red", button_type=5, no_titlebar=True)

        if event in (None, "Main Menu"):
            objects.clear()
            break

        if event == "Rescan!":
            objects.clear()
            ldr_scan.clear()
            if calibrated:
                scan = True

                graph.erase()
                graph.draw_arc((10, -160), (440, 360), 180, 0, style='arc', arc_color='green')
                graph.draw_arc((60, -110), (390, 310), 180, 0, style='arc', arc_color='green')
                graph.draw_arc((110, -60), (340, 260), 180, 0, style='arc', arc_color='green')
                graph.DrawLine((10, 100), (440, 100), width=2, color="white")
                window.hide()
                startSweep('r', s)
                scanning(scan, ldr_scan, s)
                while not calibrated:  # if received 'c' from push button during the scan
                    calibrated = calibration(scan, s)
                    if break_calibration:  # if hit Cancel in the calibration mode stay with the last calibration
                        break_calibration = False
                        calibrated = True
                        enableTX = True
                        break
                    objects.clear()
                    ldr_scan.clear()
                    scanning(scan, ldr_scan, s)
                printscan = True
                window.un_hide()
            else:
                sg.popup("need to calibrate LDRs first!!", font="any 20 bold", auto_close=True, auto_close_duration=1,
                         text_color="dark red", button_type=5, no_titlebar=True)

        if event == 'calibration':
            window.hide()
            scan = True
            startSweep('c', s)
            calibrated = calibration(scan, s)
            if break_calibration:
                break_calibration = False
                continue
            objects.clear()
            ldr_scan.clear()
            scanning(scan, ldr_scan, s)
            while not calibrated:  # if received 'c' from push button during the scan
                calibrated = calibration(scan, s)
                if break_calibration:  # if hit Cancel in the calibration mode stay with the last calibration
                    break_calibration = False
                    calibrated = True
                    sendchar('d', s)   # signal the MCU to cancel
                    break
                objects.clear()
                ldr_scan.clear()
                scanning(scan, ldr_scan, s)
                graph.erase()
                graph.draw_arc((10, -160), (440, 360), 180, 0, style='arc', arc_color='green')
                graph.draw_arc((60, -110), (390, 310), 180, 0, style='arc', arc_color='green')
                graph.draw_arc((110, -60), (340, 260), 180, 0, style='arc', arc_color='green')
                graph.DrawLine((10, 100), (440, 100), width=2, color="white")
            printscan = True
            window.un_hide()

    window.close()

