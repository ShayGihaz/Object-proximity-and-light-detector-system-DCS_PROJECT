import PySimpleGUI as sg
import serial as ser
import time
import math

import Object

Commands = {"inc_lcd": "01", "dec_lcd": "02", "rra_lcd": "03", "set_delay": "04", "clear_lcd": "05",
            "servo_deg": "06", "servo_scan": "07", "sleep": "08"}
Converted_file = []
# angle_calc = lambda angle: int((int(angle) - 350)/10)  #  1800 = 2150-350
sound_speed = (331.3 + 0.606 * 35) * 100
range_cm = lambda cycles: (sound_speed / 2) * cycles * (1/(2**20))
global s
enableTX = True
scaleX = 2
scaleY = 1.5
GRAPH_SIZE = (450*scaleX, int(450*scaleY)-100)
MCU_have_script = [0, 0, 0]
objects = []
colors = ["blue", "green", "red", "purple", "yellow", "brown", "pink", "grey", "orange", "white"]

def startSweep():
    global enableTX
    global s
    # tx to mcu to start sweep
    enableTX = True
    s.reset_output_buffer()
    s.reset_input_buffer()
    while s.out_waiting > 0 or enableTX:  # while the output buffer isn't empty
        bytetxMsg = bytes('1' + '\n', 'ascii')  # '1' for MCU is to start sweep
        s.write(bytetxMsg)
        if s.out_waiting == 0:
            enableTX = False
    #  end of sending start sweep bit


def checkfile(path):
    global Converted_file
    try:
        file = open(path, 'r')
        file_content = file.read()  # str of the whole file
    except Exception:
        print("No file path")
        return False
    temp = file_content.split("\n")
    i = 0
    while i < len(temp):
        try:
            command = temp[i].split(" ")
            if command[0] not in Commands:
                return False  # not a valid command!!
            elif command[0] == "sleep" or command[0] == "clear_lcd":
                try:
                    command[1] = 0  # checking that there is no number next to these Commands
                    return False
                except Exception:  # all good!
                    Converted_file.append(Commands.get(command[0]))
                    i = i + 1
                    continue
            elif command[0] == "servo_deg":
                if int(command[1]) < 0 or int(command[1]) > 180:
                    return False
                else:  # all good!
                    op1 = hex(int(command[1]))[2:]  # [2:] because of the '0x' we get from hex(), we remove this 0x
                    if len(op1) < 2:
                        op1 = '0' + op1
                    Converted_file.append(Commands.get(command[0]) + op1.upper())
                    i = i + 1
                    continue
            elif command[0] == "servo_scan":
                command[1] = command[1].split(",")  # if there is no argument in command[1] then the file is not good
                # and will go to the except block
                if type(command[1] is list):
                    if (int(command[1][0]) < 0 or int(command[1][0]) > 180) or (int(command[1][1]) < 0 or int(command[1][1]) > 180):
                        return False
                    else:  # all good!
                        op1 = hex(int(command[1][0]))[2:]
                        op2 = hex(int(command[1][1]))[2:]
                        if len(op1) < 2:
                            op1 = '0' + op1
                        if len(op2) < 2:
                            op2 = '0' + op2
                        Converted_file.append(Commands.get(command[0]) + op1.upper() + op2.upper())
                        i = i + 1
                        continue
                else:
                    return False
            else:
                int(command[1])  # making sure that the operand is a number!
                op1 = hex(int(command[1]))[2:]
                if len(op1) < 2:
                    op1 = '0' + op1
                Converted_file.append(Commands.get(command[0]) + op1.upper())
                i = i + 1
        except Exception:
            return False
    print(Converted_file)
    return True


def sendfile(script_num):
    global s, enableTX, Converted_file
    s.reset_output_buffer()
    s.reset_input_buffer()
    enableTX = True
    ack = False

    while s.out_waiting > 0 or enableTX:
        bytetxscript = bytes(script_num, 'ascii')
        s.write(bytetxscript)
        if s.out_waiting == 0:
            enableTX = False
    s.reset_output_buffer()
    # time.sleep(0.25)  # delay for accurate write operations on both ends
    # sending a bit to MCU to tell him that file is about to be sent!
    send_char('r')
    enableTX = True
    commands_count = len(Converted_file)
    time.sleep(0.25)  # delay for accurate read/write operations on both ends
    i = 0
    while s.out_waiting > 0 or enableTX:
        bytetxcommand = bytes(Converted_file[i], 'utf-8')  # maybe need to be utf-8 cause it is longer than a character
        s.write(bytetxcommand)
        time.sleep(0.25)  # delay for accurate read/write operations on both ends
        i = i + 1
        if i == commands_count:
            send_char('!')
            enableTX = False
    while s.in_waiting > 0 or not ack:  # ack form MCU
        byteack = s.readline()
        if byteack.decode("ascii") == 'd':
            print('received script No. ' + script_num)
            if s.in_waiting == 0:
                ack = True
                enableTX = True
                MCU_have_script[int(script_num)-1] = 1
                sg.popup("MCU got the script file!", no_titlebar=True, auto_close=True,
                         auto_close_duration=1, font="any 20", button_type=5)
                return
    #  ADD ACKNOWLEDGE RECEIVE!!!!!!


def activescript(script_num):  # make the check here, no need for communication
    global s, enableTX
    s.reset_output_buffer()
    s.reset_input_buffer()
    enableTX = True
    while s.out_waiting > 0 or enableTX:
        bytetxscript = bytes(script_num, 'ascii')
        s.write(bytetxscript)
        time.sleep(0.25)  # delay for accurate read/write operations on both ends
        if s.out_waiting == 0:
            enableTX = False
    time.sleep(0.25)  # delay for accurate read/write operations on both ends
    enableTX = True
    while s.out_waiting > 0 or enableTX:
        bytetxscript = bytes('p', 'ascii')
        s.write(bytetxscript)
        time.sleep(0.25)  # delay for accurate read/write operations on both ends
        if s.out_waiting == 0:
            enableTX = False
    if MCU_have_script[int(script_num)-1] == 1:
        return True
    else:
        sg.popup("No file in memory!", no_titlebar=True, auto_close=True, auto_close_duration=1, button_type=5,
                 text_color="red", font="any 16 bold")
        return False


def popup(message):
    """ returns a window template with nothing but text and no titlebar """
    # sg.theme('DarkGrey')
    layout = [[sg.Text(message, font="any 14", text_color="red")]]
    window = sg.Window('Message', layout, no_titlebar=True, finalize=True)
    return window


def send_char(c):
    global enableTX
    enableTX = True
    # sending 'p' to MCU as ack for receiving angle
    while s.out_waiting > 0 or enableTX:
        charbyte = bytes(c, 'ascii')
        s.write(charbyte)
        if s.out_waiting == 0:
            enableTX = False


def readcommands(running_window):
    global enableTX
    active = True
    obj_window = 0
    tele_window = 0
    tele_event = 0
    str_distance = "nothing"
    angles = []
    objects.clear()
    while active:
        running_window.refresh()
        if obj_window != 0:
            obj_window.refresh()
            running_window.refresh()
        while s.in_waiting > 0:
            bytesomething = s.read()
            something = bytesomething.decode("utf-8")
            print("opcode: ", something)
            if something == 's':  # AKA scan
                left_read = False
                left = 0
                right_read = False
                right = 0
                while not left_read:
                    while s.in_waiting > 0:
                        anglebyte = s.readline()
                        angle = anglebyte.decode("utf-8")  # reading angle from script
                        if s.in_waiting == 0:
                            left_read = True
                            left = angle
                            print("left angle received: ", str(angle))
                send_char('p')  # left angle received, continue to right angle
                while not right_read:
                    while s.in_waiting > 0:
                        anglebyte = s.readline()
                        angle = anglebyte.decode("utf-8")  # reading angle from script
                        if s.in_waiting == 0:
                            right_read = True
                            right = angle
                            print("right angle received: ", str(angle))
                angles.append((left, right))
                print(angles)
                send_char('p')  # right angle received, continue to scan
                if obj_window != 0:
                    obj_window.close()
                obj_window = object_window(angles)
                obj_window.refresh()
                print("after obj read")
                # time.sleep(1)
                send_char('p')  # distances received, continue
                print("after sending p")
                # send_char('p')  # plaster --> MCU dont wake up after 1 'p'

            elif something == 'd':  # AKA degree
                angle_read = False
                distance_read = False
                while not angle_read:
                    while s.in_waiting > 0:
                        anglebyte = s.readline()
                        angle = anglebyte.decode("utf-8")  # reading angle from script
                        if s.in_waiting == 0:
                            angle_read = True
                            print("angle received: ", str(angle))
                            enableTX = True
                            # sending 'p' to MCU as ack for receiving angle
                            while s.out_waiting > 0 or enableTX:
                                angle_ack = bytes('p', 'ascii')
                                s.write(angle_ack)
                                if s.out_waiting == 0:
                                    enableTX = False
                                    angle_read = True
                # reading distance
                while not distance_read:
                    while s.in_waiting > 0:
                        charByte = s.readline()  # expect to find '\n' in the end of the distance!
                        str_distance = int(charByte.decode("utf-8"))
                        print(str_distance)
                        if s.in_waiting == 0:
                            distance_read = True
                            enableTX = True
                if int(range_cm(str_distance)) > 0:
                    sg.popup("Know angle: " + str(angle)[:-1] + "°" + "\nObject found in: " + str(int(range_cm(str_distance)))
                            + " cm", non_blocking=True, font="any 20", auto_close=True, auto_close_duration=60)
                else:
                    sg.popup("Know angle: " + str(angle)[:-1] + "°" + "\nObject found in: bad scan",
                             non_blocking=True, font="any 20", auto_close=True, auto_close_duration=60)
                send_char('p')
            elif something == 'f':  # AKA finish
                objects.clear()
                return obj_window


def object_window(angles):
    global s, enableTX, objects, colors
    angles_screen = []
    for i in range(0, len(angles)):
        angles_screen.append(('                         left angle: ' + str(angles[i][0])[:-1] + '      right angle: '
                              + str(angles[i][1]), colors[i]))
    x_offset = 225 * scaleX
    y_offset = int(100 * scaleY) - 100
    Str_distance = "450"
    layout = [
        [sg.T("                   Object Detector System", font="any 30 bold", text_color='red',
              size=(0, 1))],
        [sg.T("")], [sg.T("                    "), sg.Multiline('', size=(100, 4), key='_ANGLES_', no_scrollbar=False)],
        [sg.T(" ", size=(1, 2))],
        [sg.Graph(canvas_size=GRAPH_SIZE, graph_top_right=GRAPH_SIZE, graph_bottom_left=(0, 0),
                  background_color='black', key="_GRAPH_")]
    ]

    scan = True
    # tx to mcu to start sweep
    startSweep()
    print(int(angles[-1][0]), int(angles[-1][1]))
    Object.scanning(scan, objects, s, int(angles[-1][0]), int(angles[-1][1]))

    objects.append('Q')  # separate the different scans one from another
    #  end
    window = sg.Window('Object Proximity Detector', layout, location=(300, 5))
    window.finalize()
    for line, color in angles_screen:
        window['_ANGLES_'].print(line, text_color=color, font="any 16")
    graph = window["_GRAPH_"]
    graph.draw_arc((10 * scaleX, int(-160 * scaleY) - 100), (440 * scaleX, int(360 * scaleY) - 100), 180, 0,
                   style='arc', arc_color='green')
    graph.draw_arc((60 * scaleX, int(-110 * scaleY) - 100), (390 * scaleX, int(310 * scaleY) - 100), 180, 0,
                   style='arc', arc_color='green')
    graph.draw_arc((110 * scaleX, int(-60 * scaleY) - 100), (340 * scaleX, int(260 * scaleY) - 100), 180, 0,
                   style='arc', arc_color='green')
    graph.DrawLine((10 * scaleX, int(100 * scaleY) - 100), (440 * scaleX, int(100 * scaleY) - 100), width=2,
                   color="white")
    color_idx = 0
    angle_distinguish = 1  # ANGLE_DISTINGUISH VARIABLE AND ALL OF IT'S LOGIC IS USED FOR SENSITIVITY OF SCANS
    for i in range(0, len(objects)):
        try:
            distance = int(objects[i][0])
            angle = math.radians(objects[i][1])
        except ValueError:
            angle_distinguish = 1
            color_idx += 1
            continue
        if objects[i - 1][0] != 'Q':
            if (distance < int(objects[i - 1][0]) + 6 and distance + 6 > int(objects[i - 1][0])) and i != 0:
                angle_distinguish += 1
                if angle_distinguish > 3:  # only 3 angles ahead --> every 9 degrees it must be a new object!
                    angle_distinguish = 1
                else:  # the scan is in the range of duplicate objects
                    continue
            else:
                angle_distinguish = 1
        if distance <= int(Str_distance) and distance > 1:
            graph.draw_text("X", location=(
                distance * math.cos(angle) * (43 / 90) * scaleX + x_offset,
                distance * math.sin(angle) * (26 / 45) * scaleY + y_offset),
                            # 225 is offset of x-axis and 43/90 = 430/900,  430 from 900/2-20
                            # +100 is the height of the base line and 26/45 = 260/450 where 260 is the max of the arc
                            font="any 14", color=colors[color_idx])
            if (math.ceil(math.degrees(angle))) > 160:
                text_offset = 15
            elif (math.ceil(math.degrees(angle))) < 20:
                text_offset = -15
            else:
                text_offset = 0
            graph.draw_text(f'({int(distance)}, {math.ceil(math.degrees(angle))}°)', location=(
                distance * math.cos(angle) * (43 / 90) * scaleX + x_offset + text_offset,
                distance * math.sin(angle) * (26 / 45) * scaleY + y_offset - 15),
                            font="any 9", color="white")
            window["_GRAPH_"].update()
    print("returning from scan window")
    return window


def ScriptMenu(com):
    global Converted_file, s
    s = com
    layout = [[sg.T("   Script Mode", font="any 30 bold", text_color='red')],
              [sg.B("Script 1", size=(10, 5), key="_S1_"), sg.B("Script 2", size=(10, 5), key="_S2_"),
               sg.B("Script 3", size=(10, 5), key="_S3_")],
              [sg.Input(key="_F1_", size=(12, 5)), sg.Input(key="_F2_", size=(12, 5)),
               sg.Input(key="_F3_", size=(12, 5))],
              # all the next buttons are the same, just different address
              [sg.FileBrowse(target="_F1_", file_types=(('Text', '*.txt'),), key="_F1BROWSE_", size=(10, 1)),
               sg.FileBrowse(target="_F2_", file_types=(('Text', '*.txt'),), key="_F2BROWSE_", size=(10, 1)),
               sg.FileBrowse(target="_F3_", file_types=(('Text', '*.txt'),), key="_F3BROWSE_", size=(10, 1))],
              [sg.Submit(size=(10, 1), key="_F1SUB_", button_color=("white", "black")),
               sg.Submit(size=(10, 1), key="_F2SUB_", button_color=("white", "black")),
               sg.Submit(size=(10, 1), key="_F3SUB_", button_color=("white", "black"))],
              [sg.B("Main Menu", pad=(110, 20))]
              ]
    obj = 0
    window = sg.Window('Script Mode', layout)

    while True:
        Converted_file.clear()  # cleaning the variable for the next load
        event, values = window.read(timeout=50, timeout_key="_TIMEOUT_")

        if event in (None, "Main Menu"):
            if obj != 0:
                obj.close()
            break

        if event == "_F1SUB_":
            if not checkfile(values["_F1_"]):
                sg.popup("File is not valid!", no_titlebar=True, auto_close=True, auto_close_duration=1, button_type=5,
                 text_color="red", font="any 16 bold")
                continue
            else:
                window.refresh()
                sg.popup("sending script file...", auto_close=True, auto_close_duration=3,
                         non_blocking=True, button_type=5, no_titlebar=True)
                sendfile('1')

        if event == "_F2SUB_":
            if not checkfile(values["_F2_"]):
                sg.popup("File is not valid!", no_titlebar=True, auto_close=True, auto_close_duration=1, button_type=5,
                 text_color="red", font="any 16 bold")
                continue
            else:
                window.refresh()
                sg.popup("sending script file...", auto_close=True, auto_close_duration=3,
                         non_blocking=True, button_type=5, no_titlebar=True)
                sendfile('2')

        if event == "_F3SUB_":
            if not checkfile(values["_F3_"]):
                sg.popup("File is not valid!", no_titlebar=True, auto_close=True, auto_close_duration=1, button_type=5,
                 text_color="red", font="any 16 bold")
                continue
            else:
                window.refresh()
                sg.popup("sending script file...", auto_close=True, auto_close_duration=3,
                         non_blocking=True, button_type=5, no_titlebar=True)
                sendfile('3')

        if event == "_S1_":
            ack = activescript('1')
            print("ack: ", ack)
            if ack:
                if obj != 0:
                    obj.close()
                    obj = 0
                window.hide()
                running_script_window = popup("running script No. 1...")
                obj = readcommands(running_script_window)
                running_script_window.close()
                window.un_hide()
                ack = False
        # ASSUMING NO ERROR OCCURRED  --> SCRIPT IS RUNNING OK  -->  need to read from MCU acknowledge  -- recieved in activescript
        if event == "_S2_":
            ack = activescript('2')
            if ack:
                if obj != 0:
                    obj.close()
                    obj = 0
                window.hide()
                running_script_window = popup("running script No. 2...")
                obj = readcommands(running_script_window)
                running_script_window.close()
                window.un_hide()
                ack = False
                # read requests from MCU when commands servo_scan and servo_deg are coming and act as well
            # ASSUMING NO ERROR OCCURRED  --> SCRIPT IS RUNNING OK
        if event == "_S3_":
            ack = activescript('3')
            if ack:
                if obj != 0:
                    obj.close()
                    obj = 0
                window.hide()
                running_script_window = popup("running script No. 3...")
                obj = readcommands(running_script_window)
                running_script_window.close()
                window.un_hide()
                ack = False
                # read requests from MCU when commands servo_scan and servo_deg are coming and act as well
            # ASSUMING NO ERROR OCCURRED  --> SCRIPT IS RUNNING OK

    window.close()
