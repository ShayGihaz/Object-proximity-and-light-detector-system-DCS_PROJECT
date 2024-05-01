import PySimpleGUI as sg
import serial as ser
import time

import main
avg_arr = [0, 0, 0]
angle_calc = lambda angle: int(int(angle) * 10 + 490)  #  1800 = 2290-490
sound_speed = (331.3 + 0.606 * 35) * 100
range_cm = lambda cycles: (sound_speed / 2) * cycles * (1/(2**20))
def AngleChange():
    """ checking weather the input is valid
            if True, returns the string of the input, else popup an error and try again """
    layout = [[sg.T('Wanted angle: '), sg.I(key='_INPUT_', size=(8, 1))],
              [sg.B('Ok'), sg.B('Cancel')]]
    window = sg.Window('AngleUpdater', layout)
    while True:
        event, values = window.read()
        if event in (None, 'Cancel'):
            window.close()
            return None
        if event == 'Ok':
            try:
                new_angle = int(values['_INPUT_'])
                if new_angle < 0 or new_angle > 180:
                    sg.popup("angle must be: \n\n- bigger then 0\n- smaller then 180\n\n Press any key to close",
                             any_key_closes=True)
                    continue
                window.close()
                return values['_INPUT_']
            except Exception:
                sg.popup("angle must be: \n\n- AN INTEGER!\n\n- bigger then 0\n- smaller then 180\n\n Press any key to close",
                         any_key_closes=True)



def Telemeter(angle,s):

    layout = [[sg.T('    ', font="any 34 bold "),
               sg.T('Telemeter', font="any 34 bold underline", text_color='red', pad=(120, 10))],

              [sg.T('', key='_ANGLE_', pad=(100, 10), font='any 14'),
               sg.B('change', button_color=('dark blue', 'white'))],
              [sg.T('', key='_DISTANCE_', pad=(100, 10), font='any 14')],
              [sg.B("Main Menu", pad=(250, 20))]
              ]

    enableTX = True
    str_angle = str(angle)
    str_distance = '0'
    window = sg.Window('Telemeter', layout, size=(600, 250))
    window.finalize()
    window['_ANGLE_'].update('Known angle: ' + str_angle + '°')
    window['_DISTANCE_'].update('Distance: ' + str_distance + ' cm')
    s.reset_input_buffer()
    avg_idx = 0
    while True:
        event, values = window.read(timeout_key="_TIMEOUT_", timeout=300)  # 0.3 sec between sends --> check it!!
        if event in (None, "Main Menu"):
            break
        if event == "_TIMEOUT_":
            while s.in_waiting > 0:
                charByte = s.readline()
                str_distance = str(charByte.decode("utf-8"))
                print(str_distance)
                if s.in_waiting == 0:
                    enableTX = True
                if avg_idx < 3:  # appending to the average array 3 samples
                    if int(str_distance) > 0:
                        avg_arr[avg_idx] = int(str_distance)
                        avg_idx += 1
                    print(avg_arr)
                else:
                    avg_idx = 0
                    # display the average of 3 samples
                    window['_DISTANCE_'].update('Distance: ' + str(int(range_cm(sum(avg_arr)/len(avg_arr)))) + ' cm')

        if event == 'change':
            str_angle_temp = AngleChange()
            if (str_angle_temp == None):
                continue
            str_angle = str(angle_calc(str_angle_temp))  # calculating angle for the required PWM high level
            print("angle: ", str_angle)
            s.reset_output_buffer()
            enableTX = True
            while s.out_waiting > 0 or enableTX:
                bytetxangle = bytes(str_angle + '\n', 'ascii')
                s.write(bytetxangle)  # sending the angle after calculating it for the required PWM high level
                if s.out_waiting == 0:
                   enableTX = False
            window['_ANGLE_'].update('Known angle: ' + str_angle_temp + '°')
    window.close()


