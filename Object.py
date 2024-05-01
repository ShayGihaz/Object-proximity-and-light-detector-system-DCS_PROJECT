import PySimpleGUI as sg
import serial as ser
import time
import math


temperature = 25
number_of_scan = 61
Str_distance = '450'
angle_calc = lambda angle: int((angle - 350) / 10)  #  1800 = 2150 -350
sound_speed = (331.3 + 0.606 * temperature) * 100
range_cm = lambda cycles: (sound_speed / 2) * cycles * (1/(2**20))  # cycles * (1/(2**20))  =  Echo high level time
margin = 1800
angle_calc_to_PWM = lambda angle: int((angle * 10) + 350)  #  1800 = 2150 -350

global s
enableTX = True
scaleX = 2
scaleY = 1.5
GRAPH_SIZE = (450*scaleX, int(450*scaleY)-100)
final_list_objects = []
objects = []


def popup_new_dis(message):
    """ returns a window template with an Ok button and no titlebar """
    layout = [[sg.Text(message)], [sg.B('Ok')]]
    window = sg.Window('Message', layout, no_titlebar=True, keep_on_top=True, finalize=True)
    return window


def popup(message):
    """ returns a window template with nothing but text and no titlebar """
    # sg.theme('DarkGrey')
    layout = [[sg.Text(message, font="any 16", text_color="red")]]
    window = sg.Window('Message', layout, no_titlebar=True, finalize=True, keep_on_top=True)
    return window


def scanning(scan, objects, s, left=0, right=180):
    """ Getting the Echo high level time in cycles and appending it with the right angle to the object array """
    global enableTX
    margin1 = angle_calc_to_PWM(right) - angle_calc_to_PWM(left)
    number_of_scan1 = int(margin1 / 30) + 1
    angle_conter = 0
    counter = 0
    scan_popup = popup("scanning objects...")
    scan_popup.refresh()
    print("start scan!")
    while scan:
        i = 0
        info = [0, 0]  # info[0] is the Echo high level time in cycles while info[1] is the angle

        while s.in_waiting > 0:  # while the input buffer isn't empty
            enableTX = False
            temp = s.readline()
            tempbyte = str(temp.decode("utf-8"))
            # print("RX: ", tempbyte)
            info[i] = (int(tempbyte))
            i = i + 1
            if i > 1:  # plaster
                print("index i in scanning is bigger then 1!\n")  # plaster debugger
                print("info: ", info, "\n")                       # plaster debugger
                print("objects: ", objects, "\n")                 # plaster debugger
                info[1] = angle_conter
                angle_conter += int((margin1 / (number_of_scan1 * 10)) + 0.5)
                real_info = (int(range_cm(info[0])), info[1])
                objects.append(real_info)
                counter += 1
                if counter == number_of_scan1:
                    print("counter: ", counter, "\n")
                    scan = False
                    break
                # break  --  maybe we are missing 1 scan. therefore we are appending here the 'lost' scan
                # if we are not missing a scan, remove the next 2 lines, de-comment the break statement above
                # and remove the break before it in the if statement
                i = 1
                info[0] = (int(tempbyte))
            info[i] = angle_conter

            angle_conter += int((margin1 / (number_of_scan1 * 10)) + 0.5)
            real_info = (int(range_cm(info[0])), info[1])
            objects.append(real_info)
            counter += 1
            if counter == number_of_scan1:
                print("counter: ", counter, "\n")
                scan = False
                break

    scan_popup.close()
    # print(objects)

def LimitChange():
    """ checking weather the input is valid
        if True, returns the string of the input, else popup an error and try again """
    layout = [[sg.T('Wanted Distance: '), sg.I(key='_INPUT_', size=(8, 1))],
              [sg.B('Ok'), sg.B('Cancel')]]
    window = sg.Window('Limit Updater', layout)
    while True:
        event, values = window.read()
        if event in (None, 'Cancel'):
            window.close()
            return None
        if event == 'Ok':
            try:
                new_limit = int(values['_INPUT_'])
                if new_limit < 0 or new_limit > 450:
                    sg.popup("Distance must be: \n\n- bigger then 0\n- smaller then 450\n\n Press any key to close",
                             any_key_closes=True)
                    continue
                window.close()
                return values['_INPUT_']
            except Exception:
                sg.popup(
                    "Distance must be: \n\n- AN INTEGER!\n\n- bigger then 0\n- smaller then 450\n\n Press any key to close",
                    any_key_closes=True)


def startSweep(com):
    """ Signals the MCU to get out of LPM mode and start the scan """
    global enableTX
    # tx to mcu to start sweep
    enableTX = True
    com.reset_output_buffer()
    com.reset_input_buffer()
    while com.out_waiting > 0 or enableTX:  # while the output buffer isn't empty
        bytetxMsg = bytes('s' + '\n', 'ascii')  # 's' for MCU is to start sweep
        com.write(bytetxMsg)
        if com.out_waiting == 0:
            enableTX = False
    #  end of sending start sweep bit

def Object(com, limit):
    global Str_distance, objects, enableTX, s
    s = com
    Str_distance = str(limit)
    x_offset = 225*scaleX
    y_offset = int(100*scaleY) - 100
    objects.clear()
    startSweep(com)
    scanning(True, objects, s)
    scan = False
    list_window = False

    # Define the layout
    layout = [
        [sg.T("                          Object Detector", font="any 30 bold", text_color='red', size=(0, 1))],
        [sg.T("                                  System", font="any 30 bold", text_color='red')],
        [sg.T("", font="any 14", key="_DISTANCE_"), sg.B('change', button_color=('dark blue', 'white'))],
        [sg.Graph(canvas_size=GRAPH_SIZE, graph_top_right=GRAPH_SIZE, graph_bottom_left=(0, 0),
                  background_color='black', key="_GRAPH_")],
        [sg.B("Rescan!"), sg.T("                            "), sg.B("Main Menu", pad=(100 * scaleX, 2)),
         sg.T("                               "), sg.B("List of Objects", key="_OBJECTS_")]
    ]

    window = sg.Window('Object Proximity Detector', layout, location=(300, 0))
    window.finalize()
    graph = window["_GRAPH_"]

    graph.draw_arc((10 * scaleX, int(-160 * scaleY) - 100), (440 * scaleX, int(360 * scaleY) - 100), 180, 0,
                   style='arc', arc_color='green')
    graph.draw_arc((60 * scaleX, int(-110 * scaleY) - 100), (390 * scaleX, int(310 * scaleY) - 100), 180, 0,
                   style='arc', arc_color='green')
    graph.draw_arc((110 * scaleX, int(-60 * scaleY) - 100), (340 * scaleX, int(260 * scaleY) - 100), 180, 0,
                   style='arc', arc_color='green')
    graph.DrawLine((10 * scaleX, int(100 * scaleY) - 100), (440 * scaleX, int(100 * scaleY) - 100), width=2,
                   color="white")
    window['_DISTANCE_'].update(
        '                                                              Limit Distance: ' + Str_distance + ' cm')


    while True:
        event, values = window.read(timeout=200, timeout_key="_TIMEOUT_")

        if event == '_TIMEOUT_' and not scan:
            final_list_objects.clear()
            angle_distinguish = 1  # ANGLE_DISTINGUISH VARIABLE AND ALL OF IT'S LOGIC IS USED FOR SENSITIVITY OF SCANS
            for i in range(0, len(objects)):
                distance = objects[i][0]
                angle = math.radians(objects[i][1])
                if (distance < objects[i-1][0] + 6 and distance + 6 > objects[i-1][0]) and i != 0:
                    angle_distinguish += 1
                    if angle_distinguish > 3:  # only 3 angles ahead --> every 9 degrees it must be a new object!
                        angle_distinguish = 1
                    else:  # the scan is in the range of duplicate objects
                        continue
                else:
                    angle_distinguish = 1
                if distance > 0:
                    final_list_objects.append((distance, objects[i][1]))
                if int(Str_distance) >= distance > 1:
                    graph.draw_text("X", location=(
                        distance * math.cos(angle) * (43/90) * scaleX + x_offset,
                        distance * math.sin(angle) * (26/45) * scaleY + y_offset),
                                    # 225 is offset of x-axis and 43/90 = 430/900,  430 from 900/2-20
                                    # +100 is the height of the base line and 26/45 = 260/450 where 260 is the max of the arc
                                    font="any 14", color="purple")
                    if (math.ceil(math.degrees(angle))) > 160:
                        text_offset = 15
                    elif (math.ceil(math.degrees(angle))) < 20:
                        text_offset = -15
                    else:
                        text_offset = 0
                    graph.draw_text(f'({int(distance)}, {math.ceil(math.degrees(angle))}°)', location=(
                        distance * math.cos(angle) * (43/90) * scaleX + x_offset + text_offset,
                        distance * math.sin(angle) * (26/45) * scaleY + y_offset - 15),
                                    font="any 9", color="white")
                    window["_GRAPH_"].update()
                scan = True

        if event in (None, "Main Menu"):
            if list_window:
                list_of_objects_window.close()
            break

        if event == "change":
            str_distance_temp = LimitChange()
            if str_distance_temp is None:
                continue
            Str_distance = str_distance_temp
            window['_DISTANCE_'].update(
                '                                                              Limit Distance: ' + Str_distance + ' cm')
            scan = True
            graph.erase()
            objects.clear()
            graph.draw_arc((10 * scaleX, int(-160 * scaleY) - 100), (440 * scaleX, int(360 * scaleY) - 100), 180, 0,
                           style='arc', arc_color='green')
            graph.draw_arc((60 * scaleX, int(-110 * scaleY) - 100), (390 * scaleX, int(310 * scaleY) - 100), 180, 0,
                           style='arc', arc_color='green')
            graph.draw_arc((110 * scaleX, int(-60 * scaleY) - 100), (340 * scaleX, int(260 * scaleY) - 100), 180, 0,
                           style='arc', arc_color='green')
            graph.DrawLine((10 * scaleX, int(100 * scaleY) - 100), (440 * scaleX, int(100 * scaleY) - 100), width=2,
                           color="white")
            if list_window:
                list_of_objects_window.close()
            enableTX = True
            startSweep(com)
            window.hide()
            scanning(scan, objects, s)
            scan = False
            window.un_hide()

        if event == "Rescan!":
            scan = True
            graph.erase()
            objects.clear()
            graph.draw_arc((10 * scaleX, int(-160 * scaleY) - 100), (440 * scaleX, int(360 * scaleY) - 100), 180, 0,
                           style='arc', arc_color='green')
            graph.draw_arc((60 * scaleX, int(-110 * scaleY) - 100), (390 * scaleX, int(310 * scaleY) - 100), 180, 0,
                           style='arc', arc_color='green')
            graph.draw_arc((110 * scaleX, int(-60 * scaleY) - 100), (340 * scaleX, int(260 * scaleY) - 100), 180, 0,
                           style='arc', arc_color='green')
            graph.DrawLine((10 * scaleX, int(100 * scaleY) - 100), (440 * scaleX, int(100 * scaleY) - 100), width=2,
                           color="white")
            enableTX = True
            if list_window:
                list_of_objects_window.close()
            startSweep(com)
            window.hide()
            scanning(scan, objects, s)
            scan = False
            window.un_hide()

        if event == "_OBJECTS_":
            if not list_window:
                # print(final_list_objects)
                string_list = ""
                for i in range(0, len(final_list_objects)):
                    if 1 < final_list_objects[i][0] <= int(Str_distance):
                        string_list = string_list + "distance: " + str(final_list_objects[i][0]) + " cm,  angle: " + str(final_list_objects[i][1]) + "°\n"

                list_of_objects_layout = [[sg.T("List Of Objects", font="any 20 bold underline", text_color="red")],
                                          [sg.Multiline(string_list, size=(40, 10), text_color='white',
                                                        background_color="blue")], [sg.B("close")]]
                list_of_objects_window = sg.Window('Objects list', list_of_objects_layout)
                window.hide()
                Oevent, Oval = list_of_objects_window.read()
                list_window = True

        if list_window:  # relevant only if the list of object window is open
            if Oevent in (None, "close"):
                list_of_objects_window.close()
                list_window = False
                window.un_hide()

    window.close()
