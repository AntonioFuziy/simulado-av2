import serial
import argparse
import time
import logging
import threading

#python maquininha.py COM11 -b 115200 

class SerialControllerInterface:
    def __init__(self, port, baudrate):
        self.ser = serial.Serial(port, baudrate=baudrate)
        self.incoming = '0'
        self.status = ''
        self.HEAD = b'P'
        self.OK = b'\xFF'
        self.FAIL = b'\x00'
        self.WAITING = b'\x01'
        self.CANCELED = b'\x02'
        self.REPLY_COMMAND = b'\x22'
        self.EOP = b'X'


    def background(self):
        while True:
            if self.status == '':
                print("Aguardando")
            elif self.status == 'Input':
                print("Cobrança no valor R$ {}".format(int.from_bytes(self.valor, byteorder='big'))) 
                print("Digite:")
                print("P para pagar")
                print("C para cancelar")
                u = input()
                if u[0] == 'P':
                    print('Pagamento realizado')
                    self.status = 'Pago'
                else:
                    print('Pagamento cancelado')
                    self.status = 'Cancelado'
            time.sleep(3)
            print(self.status)


    def send(self, data):
        r = self.ser.write(self.HEAD)
        r = self.ser.write(self.REPLY_COMMAND)
        r = self.ser.write(data)
        r = self.ser.write(self.EOP)
        #print(f"dado de resposta: {data}")


    def update(self):
        while(True): 
            while self.incoming != self.EOP:
                self.incoming = self.ser.read()
                logging.debug("Received INCOMING: {}".format(self.incoming))
                #print(f"dado novo: {self.incoming}")
            
            package = {}
            package['head'] = self.ser.read(1)
            package['command'] = self.ser.read(1)
            package['data'] = self.ser.read()
            print(package)

            if package['head'] != b'U':
                self.send(self.FAIL)
                print("fail")
            elif package['command'] == b'\x00':
                self.send(self.OK)
                print("ok")
            elif package['command'] == b'\x01':
                if self.status == 'Input' or self.status == 'Pago':
                    print("falhou")
                    self.send(self.FAIL)
                else:
                    print("input")
                    self.valor = package['data']
                    self.status = 'Input'
                    self.send(self.OK)
            elif package['command'] == b'\x02':
                if self.status == 'Input':
                    self.send(self.WAITING)
                elif self.status == 'Pago':
                    self.status = ''
                    self.send(self.OK)
                elif self.status == 'Cancelado':
                    self.status = ''
                    self.send(self.CANCELED)
                else:
                    self.send(self.FAIL)

            self.incoming = self.ser.read()


if __name__ == '__main__':
    argparse = argparse.ArgumentParser()
    argparse.add_argument('serial_port', type=str, default='COM3')
    argparse.add_argument('-b', '--baudrate', type=int, default=115200)
    argparse.add_argument('-d', '--debug', default=False, action='store_true')
    args = argparse.parse_args()

    if args.debug:
        logging.basicConfig(level=logging.DEBUG)

    print("Connection to {} with {})".format(args.serial_port, args.baudrate))
    maquininha = SerialControllerInterface(args.serial_port, args.baudrate)

    # now threading1 runs regardless of user input
    threading1 = threading.Thread(target=maquininha.update)
    threading1.daemon = True
    threading1.start()

    while True:
        maquininha.background()
