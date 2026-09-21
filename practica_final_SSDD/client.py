from enum import Enum
import argparse
import socket
import threading
from ws_normalizador_client import normalizar_mensaje

class client :

    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    _server = None
    _port = -1
    mi_nombre_usuario = None
    socket_escucha = None
    usuarios_conectados = {}

    # ******************** METHODS *******************

    # Método de hilo de escucha
    @staticmethod
    def recibidor_mensajes(socket_escucha):
        # Siempre activo
        while True:
            try:
                # El hilo se queda aquí esperando mensajes de otros usuarios
                socket_temporal, addr = socket_escucha.accept() # addr es tupla de IP, puerto de quién manda el mensaje

                # Obtenemos la operacion correspondiente
                operacion = ""
                while True:
                    char = socket_temporal.recv(1).decode()
                    if char=='\0':
                        break
                    operacion += char
                
                # Primer caso: servidor nos trae mensaje de otro usuario
                if operacion=="SEND_MESSAGE":
                    # Obtenemos nombre del remitente
                    remitente = ""
                    while True:
                        char = socket_temporal.recv(1).decode()
                        if char=='\0':
                            break
                        remitente += char
                    
                    # Obtenemos id (QUE EN PRINCIPIO ES UN STR PREGUNTAR AL PROFE)
                    id = ""
                    while True:
                        char = socket_temporal.recv(1).decode()
                        if char=='\0':
                            break
                        id += char
                    
                    # Obtenemos el mensaje
                    mensaje = ""
                    while True:
                        char = socket_temporal.recv(1).decode()
                        if char=='\0':
                            break
                        # Controlamos longitud
                        if len(mensaje)<255:
                            mensaje += char
                    print(f"MESSAGE {id} FROM {remitente}")
                    print(mensaje)
                    print("END")
                    print("c> ", end="", flush=True)    # Para que el usuario pueda seguir escribiendo
                
                # Segundo caso: confirmación de que un mensaje nuestro se ha entregado
                elif operacion=="SEND_MESS_ACK":
                    #Obtenemos el id
                    id = ""
                    while True:
                        char = socket_temporal.recv(1).decode()
                        if char=='\0':
                            break
                        id += char
                    print(f"SEND MESSAGE {id} OK")
                    print("c> ", end="", flush=True)    # Para que el usuario pueda seguir escribiendo

                # Tercer caso: servidor nos trae mensaje con file de otro usuario
                elif operacion=="SEND_MESSAGE_ATTACH":
                    # Obtenemos nombre del remitente
                    remitente = ""
                    while True:
                        char = socket_temporal.recv(1).decode()
                        if char=='\0':
                            break
                        remitente += char
                    
                    # Obtenemos id 
                    id = ""
                    while True:
                        char = socket_temporal.recv(1).decode()
                        if char=='\0':
                            break
                        id += char
                    
                    # Obtenemos el mensaje
                    mensaje = ""
                    while True:
                        char = socket_temporal.recv(1).decode()
                        if char=='\0':
                            break
                        # Controlamos longitud
                        if len(mensaje)<255:
                            mensaje += char

                    # Obtenemos el nombre del archivo adjunto
                    archivo = ""
                    while True:
                        char = socket_temporal.recv(1).decode()
                        if char=='\0':
                            break
                        archivo += char

                    print(f"MESSAGE {id} FROM {remitente}")
                    print(mensaje)
                    print("END")
                    print(f"FILE {archivo}")
                    print("c> ", end="", flush=True)    # Para que el usuario pueda seguir escribiendo

                # Cuarto caso: confirmación de que un mensaje con file nuestro se ha entregado
                elif operacion=="SEND_MESS_ATTACH_ACK":
                    #Obtenemos el id
                    id = ""
                    while True:
                        char = socket_temporal.recv(1).decode()
                        if char=='\0':
                            break
                        id += char
                    # Obtenemos el nombre del archivo adjunto
                    archivo = ""
                    while True:
                        char = socket_temporal.recv(1).decode()
                        if char=='\0':
                            break
                        archivo += char
                    print(f"SENDATTACH MESSAGE {id} {archivo} OK")
                    print("c> ", end="", flush=True)    # Para que el usuario pueda seguir escribiendo
                
                # Quinto caso: otro usuario nos pide un fichero
                elif operacion=="GETFILE":
                    # quien pide
                    solicitante = ""
                    while True:
                        char = socket_temporal.recv(1).decode()
                        if char=='\0':
                            break
                        solicitante += char

                    # fichero solicitado
                    filename = ""
                    while True:
                        char = socket_temporal.recv(1).decode()
                        if char=='\0':
                            break
                        filename += char

                    # Enviar fichero
                    try:
                        with open(filename, "rb") as f:
                            while True:
                                data = f.read(1024)
                                if not data:
                                    break
                                socket_temporal.sendall(data)
                    except:
                        pass  # si falla, no enviamos nada

                socket_temporal.close() # Cerramos socket

            except:
                break
    
    # *
    # * @param user - User name to register in the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user is already registered
    # * @return ERROR if another error occurred
    @staticmethod
    def  register(user) :
        # Hacemos try/except para controlar posibles errores externos (como servidor caído, etc)
        try:
            # Creamos el socket
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

            # Conectamos al servidor
            sock.connect((client._server, client._port))

            # Preparamos mensaje según protocolo
            # Mandamos de uno en uno para que sea más fácil para el servidor
            # Hacemos .encode() para que se pueda entender en C
            codigo_operacion = "REGISTER\0"
            sock.sendall(codigo_operacion.encode())
            if len(user)>255:    # Comprobamos que el user tiene la longitud adecuada
                print("c> REGISTER FAIL")
                return client.RC.ERROR
            sock.sendall((user+'\0').encode())

            # Recibir respuesta
            respuesta = sock.recv(1)    # Recibimos un byte
            codigo_respuesta = int.from_bytes(respuesta, byteorder='little')   # Transformamos byte a int

            # Cerramos el socket
            sock.close()

            # Devolvemos el mensaje correspondiente
            if codigo_respuesta==0:
                print("c> REGISTER OK")
                return client.RC.OK
            elif codigo_respuesta==1:
                print("c> USERNAME IN USE")
                return client.RC.USER_ERROR
            else:
                print("c> REGISTER FAIL")
                return client.RC.ERROR
        
        except Exception as e:
            print("c> REGISTER FAIL")
            return client.RC.ERROR


    # *
    # 	 * @param user - User name to unregister from the system
    # 	 * 
    # 	 * @return OK if successful
    # 	 * @return USER_ERROR if the user does not exist
    # 	 * @return ERROR if another error occurred
    @staticmethod
    def  unregister(user) :
        # Hacemos try/except para controlar posibles errores externos (como servidor caído, etc)
        try:
            # Creamos el socket
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

            # Conectamos al servidor
            sock.connect((client._server, client._port))

            # Preparamos mensaje según protocolo
            # Mandamos de uno en uno para que sea más fácil para el servidor
            # Hacemos .encode() para que se pueda entender en C
            codigo_operacion = "UNREGISTER\0"
            sock.sendall(codigo_operacion.encode())
            if len(user)>255:    # Comprobamos que el user tiene la longitud adecuada
                print("c> UNREGISTER FAIL")
                return client.RC.ERROR
            sock.sendall((user+'\0').encode())

            # Recibir respuesta
            respuesta = sock.recv(1)    # Recibimos un byte
            codigo_respuesta = int.from_bytes(respuesta, byteorder='little')   # Transformamos byte a int

            # Cerramos el socket  
            sock.close()

            # Devolvemos el mensaje correspondiente
            if codigo_respuesta==0:
                print("c> UNREGISTER OK")
                return client.RC.OK
            elif codigo_respuesta==1:
                print("c> USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            else:
                print("c> UNREGISTER FAIL")
                return client.RC.ERROR

        except Exception as e:
            print("c> UNREGISTER FAIL")
            return client.RC.ERROR


    # *
    # * @param user - User name to connect to the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or if it is already connected
    # * @return ERROR if another error occurred
    @staticmethod
    def  connect(user) :
        #  Hacemos try/except para controlar posibles errores externos (como servidor caído, etc)
        try:
            # Buscamos el puerto libre
            client.socket_escucha = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client.socket_escucha.bind(('', 0))    # Ponemos 0 para que el sistema elija un puerto libre
            client.socket_escucha.listen(1)    # Esperamos a recibir el número de puerto
            puerto_libre = client.socket_escucha.getsockname()[1]  # Obtenemos el puerto

            # Lanzamos el hilo recibidor
            # daemon=True para que el hilo muera si cerramos el programa principal
            hilo_escucha = threading.Thread(target=client.recibidor_mensajes, args=(client.socket_escucha,), daemon=True)
            hilo_escucha.start()

            # Creamos el socket
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

            # Conectamos al servidor
            sock.connect((client._server, client._port))

            # Preparamos mensaje según protocolo
            # Mandamos de uno en uno para que sea más fácil para el servidor
            # Hacemos .encode() para que se pueda entender en C
            codigo_operacion = "CONNECT\0"
            sock.sendall(codigo_operacion.encode())
            if len(user)>255:    # Comprobamos que el user tiene la longitud adecuada
                print("c> CONNECT FAIL")
                return client.RC.ERROR
            sock.sendall((user+'\0').encode())
            sock.sendall((str(puerto_libre) + '\0').encode())

            # Recibir respuesta
            respuesta = sock.recv(1)    # Recibimos un byte
            codigo_respuesta = int.from_bytes(respuesta, byteorder='little')   # Transformamos byte a int

            # Cerramos el socket
            sock.close()

            # Devolvemos el mensaje correspondiente
            if codigo_respuesta==0:
                client.mi_nombre_usuario = user # Guardamos el nombre de usuario para futuras operaciones (SEND, USERS, DISCONNECT)
                print("c> CONNECT OK")
                return client.RC.OK
            elif codigo_respuesta==1:
                print("c> CONNECT FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            elif codigo_respuesta==2:
                print("c> USER ALREADY CONNECTED")
                return client.RC.USER_ERROR
            else:
                print("c> CONNECT FAIL")
                return client.RC.ERROR

        except Exception as e:
            print("c> CONNECT FAIL")
            return client.RC.ERROR

    # *
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or if it is already connected
    # * @return ERROR if another error occurred
    @staticmethod
    def  users() :
        # Hacemos try/except para controlar posibles errores externos (como servidor caído, etc)
        try:
            if client.mi_nombre_usuario is None:
                print("c> CONNECTED USERS FAIL, USER NOT CONNECTED")
                return client.RC.USER_ERROR
            client.usuarios_conectados.clear() 
            # Creamos el socket
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

            # Conectamos al servidor
            sock.connect((client._server, client._port))

            # Preparamos mensaje según protocolo
            # Mandamos de uno en uno para que sea más fácil para el servidor
            # Hacemos .encode() para que se pueda entender en C
            codigo_operacion = "USERS\0"
            sock.sendall(codigo_operacion.encode())
            sock.sendall((client.mi_nombre_usuario+'\0').encode())  # Internamente debemos mandar el nombre de usuario
                                                                    # de aquel que hace USERS

            # Recibir respuesta
            respuesta = sock.recv(1)    # Recibimos un byte
            codigo_respuesta = int.from_bytes(respuesta, byteorder='little')   # Transformamos byte a int

            # Devolvemos lo que corresponda
            if codigo_respuesta==0:
                # Creamos str que va guardando caracteres hasta que se recibe '\0' 
                # Después, transformamos a int y obtenemos número de usuarios conectados
                cantidad_str = ""
                while True:
                    char = sock.recv(1).decode()
                    if char=='\0': 
                        break
                    cantidad_str += char

                numero_usuarios = int(cantidad_str)
                print(f"c> CONNECTED USERS ({numero_usuarios} users connected) OK")

                # El servidor devuelve número de usuarios conectados (ya obtenidos) y el nombre de todos
                # Obtenemos los nombres uno a uno
                for i in range(numero_usuarios):
                    dato = ""
                    while True:
                        char = sock.recv(1).decode()
                        if char=='\0':
                            break
                        dato += char

                    nombre, ip, puerto = dato.split("::")
                    client.usuarios_conectados[nombre] = (ip, int(puerto))

                    print(f"{nombre}::{ip}::{puerto}")
                sock.close()
                return client.RC.OK
            
            elif codigo_respuesta==1:
                print("c> CONNECTED USERS FAIL, USER IS NOT CONNECTED")
                sock.close()
                return client.RC.USER_ERROR
            else:
                print("c> CONNECTED USERS FAIL")
                sock.close()
                return client.RC.ERROR

        except Exception as e:
            print("c> CONNECTED USERS FAIL")
            return client.RC.ERROR



    # *
    # * @param user - User name to disconnect from the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist
    # * @return ERROR if another error occurred
    @staticmethod
    def  disconnect(user) :
        # Hacemos try/except para controlar posibles errores externos (como servidor caído, etc)
        try:
            # Creamos el socket
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

            # Conectamos al servidor
            sock.connect((client._server, client._port))

            # Preparamos mensaje según protocolo
            # Mandamos de uno en uno para que sea más fácil para el servidor
            # Hacemos .encode() para que se pueda entender en C
            codigo_operacion = "DISCONNECT\0"
            sock.sendall(codigo_operacion.encode())
            if len(user)>255:    # Comprobamos que el user tiene la longitud adecuada
                print("c> DISCONNECT FAIL")
                return client.RC.ERROR
            sock.sendall((user+'\0').encode())

            # Recibir respuesta
            respuesta = sock.recv(1)    # Recibimos un byte
            codigo_respuesta = int.from_bytes(respuesta, byteorder='little')   # Transformamos byte a int

            # Cerramos el socket
            sock.close()

            # Devolvemos el mensaje correspondiente
            # Cerramos el hilo de escucha y lo inicializamos a None pase lo que pase
            if codigo_respuesta==0:
                print("c> DISCONNECT OK")
                if client.socket_escucha is not None:
                    client.socket_escucha.close()
                    client.socket_escucha = None
                client.mi_nombre_usuario = None # Limpiamos el usuario
                return client.RC.OK

            elif codigo_respuesta==1:
                print("c> DISCONNECT FAIL, USER DOES NOT EXIST")
                if client.socket_escucha is not None:
                    client.socket_escucha.close()
                    client.socket_escucha = None
                client.mi_nombre_usuario = None
                return client.RC.USER_ERROR
            elif codigo_respuesta==2:
                print("c> DISCONNECT FAIL, USER NOT CONNECTED")
                if client.socket_escucha is not None:
                    client.socket_escucha.close()
                    client.socket_escucha = None
                client.mi_nombre_usuario = None
                return client.RC.USER_ERROR
            else:
                print("c> DISCONNECT FAIL")
                if client.socket_escucha is not None:
                    client.socket_escucha.close()
                    client.socket_escucha = None
                client.mi_nombre_usuario = None
                return client.RC.ERROR

        except Exception as e:
            print("c> DISCONNECT FAIL")
            return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param message - Message to be sent
    # * 
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def  send(user,  message) :
        # Hacemos try/except para controlar posibles errores externos (como servidor caído, etc)
        try:
            message = normalizar_mensaje(message)
            if len(message)>255:    # Comprobamos que el mensaje tiene la longitud adecuada
                print("c> SEND FAIL")
                return client.RC.ERROR

            # Creamos el socket
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

            # Conectamos al servidor
            sock.connect((client._server, client._port))

            # Preparamos mensaje según protocolo
            # Mandamos de uno en uno para que sea más fácil para el servidor
            # Hacemos .encode() para que se pueda entender en C
            codigo_operacion = "SEND\0"
            sock.sendall(codigo_operacion.encode())
            sock.sendall((client.mi_nombre_usuario+'\0').encode())  # Internamente debemos mandar el nombre de usuario
                                                                    # de aquel que hace SEND
            if len(user)>255:    # Comprobamos que el user tiene la longitud adecuada
                print("c> SEND FAIL")
                return client.RC.ERROR
            sock.sendall((user+'\0').encode())  # Este usuario es el destinatario

            sock.sendall((message+'\0').encode())

            # Recibir respuesta
            respuesta = sock.recv(1)    # Recibimos un byte
            codigo_respuesta = int.from_bytes(respuesta, byteorder='little')   # Transformamos byte a int

            # Devolvemos el mensaje correspondiente
            if codigo_respuesta==0:
                id = ""
                while True:
                    char = sock.recv(1).decode()
                    if char == '\0':
                        break
                    id += char
                print(f"c> SEND OK - MESSAGE {id}")
                sock.close()
                return client.RC.OK
            elif codigo_respuesta==1:
                print("c> SEND FAIL, USER DOES NOT EXIST")
                sock.close()
                return client.RC.USER_ERROR
            else:
                print("c> SEND FAIL")
                sock.close()
                return client.RC.ERROR

        except Exception as e:
            print("c> SEND FAIL")
            return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param file    - file  to be sent
    # * @param message - Message to be sent
    # * 
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def  sendAttach(user,  message,  file) :
        # Hacemos try/except para controlar posibles errores externos (como servidor caído, etc)
        try:
            message = normalizar_mensaje(message)
            if len(message)>255:    # Comprobamos que el mensaje tiene la longitud adecuada
                print("c> SENDATTACH FAIL")
                return client.RC.ERROR

            # Creamos el socket
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

            # Conectamos al servidor
            sock.connect((client._server, client._port))

            # Preparamos mensaje según protocolo
            # Mandamos de uno en uno para que sea más fácil para el servidor
            # Hacemos .encode() para que se pueda entender en C
            codigo_operacion = "SENDATTACH\0"
            sock.sendall(codigo_operacion.encode())
            sock.sendall((client.mi_nombre_usuario+'\0').encode())  # Internamente debemos mandar el nombre de usuario
                                                                    # de aquel que hace SEND
            if len(user)>255:    # Comprobamos que el user tiene la longitud adecuada
                print("c> SENDATTACH FAIL")
                return client.RC.ERROR
            sock.sendall((user+'\0').encode())  # Este usuario es el destinatario

            sock.sendall((message+'\0').encode())
            if len(file)>255:    # Comprobamos que el file tiene la longitud adecuada
                print("c> SENDATTACH FAIL")
                return client.RC.ERROR
            if not file.startswith('/'):    # Comprobamos que el file es un path absoluto
                print("c> SENDATTACH FAIL")
                return client.RC.ERROR
            sock.sendall((file+'\0').encode())   # Mandamos el nombre del archivo como un string terminado en '\0'

            # Recibir respuesta
            respuesta = sock.recv(1)    # Recibimos un byte
            codigo_respuesta = int.from_bytes(respuesta, byteorder='little')   # Transformamos byte a int

            # Devolvemos el mensaje correspondiente
            if codigo_respuesta==0:
                id = ""
                while True:
                    char = sock.recv(1).decode()
                    if char == '\0':
                        break
                    id += char
                print(f"c> SENDATTACH OK - MESSAGE {id}")
                sock.close()
                return client.RC.OK
            elif codigo_respuesta==1:
                print("c> SENDATTACH FAIL, USER DOES NOT EXIST")
                sock.close()
                return client.RC.USER_ERROR
            else:
                print("c> SENDATTACH FAIL")
                sock.close()
                return client.RC.ERROR

        except Exception as e:
            print("c> SENDATTACH FAIL")
            return client.RC.ERROR
        

    
    @staticmethod
    def getfile(user, remote_file, local_file):
        try:
            # Actualizar users conectados
            client.users()

            # Comprobar que el usuario está conectado
            if user not in client.usuarios_conectados:
                print("c> FILE TRANSFER FAILED, user not connected.")
                return client.RC.USER_ERROR

            # Tomamos ip y puerto del user
            ip, puerto = client.usuarios_conectados[user]

            # Conectar al otro cliente
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((ip, puerto))

            # 1. operación
            sock.sendall("GETFILE\0".encode())

            # 2. quien pide
            sock.sendall((client.mi_nombre_usuario + '\0').encode())

            # 3. nombre fichero remoto
            sock.sendall((remote_file + '\0').encode())

            # 4. recibir fichero
            with open(local_file, "wb") as f:
                while True:
                    data = sock.recv(1024)
                    if not data:
                        break
                    f.write(data)

            sock.close()
            print("c> FILE RECEIVED OK")

            return client.RC.OK

        except Exception as e:
            print("Error:", str(e))
            return client.RC.ERROR
    


    # *
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():

        while (True) :
            try :
                command = input("c> ")
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0]=="REGISTER") :
                        if (len(line) == 2) :
                            client.register(line[1])
                        else :
                            print("Syntax error. Usage: REGISTER <userName>")

                    elif(line[0]=="UNREGISTER") :
                        if (len(line) == 2) :
                            client.unregister(line[1])
                        else :
                            print("Syntax error. Usage: UNREGISTER <userName>")

                    elif(line[0]=="CONNECT") :
                        if (len(line) == 2) :
                            client.connect(line[1])
                        else :
                            print("Syntax error. Usage: CONNECT <userName>")

                    elif(line[0]=="DISCONNECT") :
                        if (len(line) == 2) :
                            client.disconnect(line[1])
                        else :
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif(line[0]=="USERS") :
                        if (len(line) == 1) :
                            client.users()
                        else :
                            print("Syntax error. Usage: CONNECTED_USERS <userName>")

                    elif(line[0]=="SEND") :
                        if (len(line) >= 3) :
                            #  Remove first two words
                            message = ' '.join(line[2:])
                            client.send(line[1], message)
                        else :
                            print("Syntax error. Usage: SEND <userName> <message>")

                    elif(line[0]=="SENDATTACH") :
                        if (len(line) >= 4) :
                            # Formato: SENDATTACH <userName> <message> <fileName>
                            message = ' '.join(line[2:-1])
                            file = line[-1]
                            client.sendAttach(line[1], message, file)
                        else :
                            print("Syntax error. Usage: SENDATTACH <userName> <message> <fileName>")

                    elif(line[0]=="QUIT") :
                        if (len(line) == 1) :
                            break
                        else :
                            print("Syntax error. Use: QUIT")
                    elif(line[0]=="GETFILE"):
                        if (len(line) == 4):
                            client.getfile(line[1], line[2], line[3])
                        else:
                            print("Syntax error. Usage: GETFILE <userName> <fileName> <localFileName>")
                    else :
                        print("Error: command " + line[0] + " not valid.")
            except Exception as e:
                print("Exception: " + str(e))

    # *
    # * @brief Prints program usage
    @staticmethod
    def usage() :
        print("Usage: python3 client.py -s <server> -p <port>")


    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def  parseArguments(argv) :
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535")
            return False
        
        client._server = args.s
        client._port = args.p

        return True


    # ******************** MAIN *********************
    @staticmethod
    def main(argv) :
        if (not client.parseArguments(argv)) :
            client.usage()
            return

        #  Write code here
        client.shell()
        print("+++ FINISHED +++")
    

if __name__=="__main__":
    client.main([])
