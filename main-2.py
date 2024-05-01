import PySimpleGUI as sg
import Telemeter
import script
import serial as ser
import serial.tools.list_ports
import time
import Object
import light
import lightNobjects


enableTX = True
def autoPort():
    """ Detecting the MSP430 UART automatically
        if not found, program do not start and exit with exit code 0 """
    ports = serial.tools.list_ports.comports(include_links=False)
    for port in ports:
        if 'USB-to-Serial Comm Port' in port.description:
            print("found MSP430 UART port: ", port)
            # Connection to port
            s = ser.Serial(port.device,baudrate=9600, bytesize=ser.EIGHTBITS,
                   parity=ser.PARITY_NONE, stopbits=ser.STOPBITS_ONE,
                   timeout=1)
            # timeout of 1 sec where the read and write operations are blocking,
            # after the timeout the program continues
    try:
        print('Connect ' + s.name)
        return s
    except Exception:
        print("MSP430 UART was not found!\nExiting.....")
        exit()



def sendstate(state, s):
    """ sending 1 char to MCU
        used to send the correct state before activating the state function
        and so send 'Z' to MCU, to go back to state0 and main menu """
    global enableTX
    s.reset_output_buffer()
    while (s.out_waiting > 0 or enableTX):
        bytetxstate = bytes(state + '\n', 'ascii')
        s.write(bytetxstate)
        if s.out_waiting == 0:
            enableTX = False


def main():
    s = autoPort()
    s.reset_input_buffer()
    global enableTX
    layout = [[sg.T("DCS - Final Project", font="any 30 italic underline", pad=(249, 10), text_color='red')],
              [sg.T("     Light Source and Object proximity\n                   detector system", font="any 34 bold",
                    text_color='blue')],
              [sg.B("Object\nDetector\nSystem", key="_OBJECT_", size=(10, 5), button_color=("white", "red"),
                    pad=(100, 70)),
               sg.B("Telemeter", key="_TELEMETER_", size=(10, 5), button_color=("white", "red"), pad=(90, 70)),
               sg.B("Light Source\nand Object\nDetector", key="_OBJECT&LIGHT_", size=(10, 5),
                    button_color=("white", "red"), pad=(104, 70))],
              [sg.B("Light Source\nDetector\nSystem", key="_LIGHT_", size=(10, 5), button_color=("white", "red"),
                    pad=(205, 5)),
               sg.B("Script\nMode", key="_SCRIPT_", size=(10, 5), button_color=("white", "red"), pad=(25, 5))],
              [sg.B("Exit", key="_EXIT_", size=(10, 2), pad=(370, 50))]]

    window = sg.Window("Light Source and Object proximity", layout, location=(350, 50), size=(900, 700))

    while True:
        event, values = window.read(timeout=50, timeout_key="_TIMEOUT_")
        if event in (None, "_EXIT_"):
            sg.popup("goodbye!", auto_close=True, auto_close_duration=0.5, no_titlebar=True, font="any 30 italic",
                     button_type=5, text_color="purple", background_color='white')
            break

        if event == "_OBJECT_":  # state 1
            window.hide()
            distance = Object.LimitChange()
            if distance:  # if hitting the cancel button, angle equals None and nothing happened
                enableTX = True
                sendstate('1', s)
                time.sleep(0.25)
                Object.Object(s, distance)
                enableTX = True
                sendstate('Z', s)  # end of state! --> in MCU back to state 0
                time.sleep(0.25)
            window.un_hide()

        if event == "_TELEMETER_":  # state 2
            window.hide()
            angle = Telemeter.AngleChange()
            if angle:  # if hitting the cancel button, angle equals None and nothing happened
                enableTX = True
                sendstate('2', s)
                time.sleep(0.25)
                enableTX = True
                print("angle: ", Telemeter.angle_calc(angle))  # for debugging
                sendstate(str(Telemeter.angle_calc(angle)), s)  # first time the angle is sent!
                Telemeter.Telemeter(angle, s)
                enableTX = True
                sendstate('Z', s)  # end of state! --> in MCU back to state 0
                time.sleep(0.25)
            window.un_hide()

        if event == "_LIGHT_":  # state 3
            window.hide()
            enableTX = True
            sendstate('3', s)
            light.light(s)
            enableTX = True
            sendstate('Z', s)  # end of state! --> in MCU back to state 0
            window.un_hide()

        if event == "_OBJECT&LIGHT_":  # state 4
            distance = Object.LimitChange()
            if distance:  # if hitting the cancel button, angle equals None and nothing happened
                window.hide()
                enableTX = True
                sendstate('4', s)
                lightNobjects.lights_objects(s, distance)
                enableTX = True
                sendstate('Z', s)  # end of state! --> in MCU back to state 0
            window.un_hide()

        if event == "_SCRIPT_":  # state 5
            window.hide()
            enableTX = True
            sendstate('5', s)
            script.ScriptMenu(s)
            enableTX = True
            sendstate('Z', s)  # end of state! --> in MCU back to state 0
            window.un_hide()

        if event == "_TIMEOUT_":  # waiting for a signal from MCU --> push button 0
            while s.in_waiting > 0:
                enableTX = False
                temp = s.readline()
                char = temp.decode("ascii")
                print("main RX: ", char)
                if char[0] == 'c':
                    light.calibrated = light.calibration(True, s)
    window.close()
    s.close()


if __name__ == '__main__':
    main()

