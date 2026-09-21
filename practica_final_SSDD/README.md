# practica_final_SSDD

Servicio de envio de mensajes con sockets TCP, adjuntos entre clientes,
normalizacion SOAP y registro de operaciones mediante ONC-RPC.

## Dependencias

En Linux/WSL se necesitan `gcc`, `make`, `rpcgen`, `libtirpc-dev`, `rpcbind`,
`rpcsvc-proto`, Python 3 y las dependencias Python del servicio web.

### Opcion A: entorno virtual Python

Desde la raiz del proyecto:

```bash
python3 -m venv venv
source venv/bin/activate
python -m pip install -r requirements.txt
```

Con esta opcion, todas las ordenes Python deben ejecutarse con el entorno
activado, usando `python`:

```bash
python ws_normalizador_service.py
python client.py -s 127.0.0.1 -p 2000
```

### Opcion B: paquetes del sistema

Si se prefiere no usar entorno virtual:

```bash
sudo apt update
sudo apt install -y gcc make rpcbind rpcsvc-proto libtirpc-dev python3-spyne python3-zeep python3-lxml
```

Con esta opcion se usa `python3`:

```bash
python3 ws_normalizador_service.py
python3 client.py -s 127.0.0.1 -p 2000
```

## Compilacion

Desde la raiz del proyecto:

```bash
make clean
make all
```

Se generan dos ejecutables:

- `server`: servidor principal de mensajeria.
- `servidor_log`: servidor ONC-RPC que imprime las operaciones recibidas.

## Ejecucion

Abrir una terminal para cada proceso.

1. Arrancar `rpcbind`:

```bash
sudo mkdir -p /run/sendsigs.omit.d/
sudo /etc/init.d/rpcbind restart
```

2. Arrancar el servidor RPC de logs:

```bash
./servidor_log
```

3. Arrancar el servidor principal indicando donde esta el RPC:

```bash
export LOG_RPC_IP=127.0.0.1
./server -p 2000
```

4. Arrancar el servicio web normalizador en cada maquina donde se ejecute un cliente.

Si se instalo con entorno virtual:

```bash
source venv/bin/activate
python ws_normalizador_service.py
```

Si se instalaron paquetes del sistema:

```bash
python3 ws_normalizador_service.py
```

El WSDL queda disponible en:

```text
http://localhost:8000/?wsdl
```

5. Arrancar uno o varios clientes.

Si se instalo con entorno virtual:

```bash
source venv/bin/activate
python client.py -s 127.0.0.1 -p 2000
```

Si se instalaron paquetes del sistema:

```bash
python3 client.py -s 127.0.0.1 -p 2000
```

## Comandos del cliente

```text
REGISTER <userName>
UNREGISTER <userName>
CONNECT <userName>
DISCONNECT <userName>
USERS
SEND <userName> <message>
SENDATTACH <userName> <message> <fileName>
GETFILE <userName> <fileName> <localFileName>
QUIT
```

Para `SENDATTACH`, `fileName` debe ser una ruta absoluta. Para recuperar el
contenido del fichero se usa `GETFILE`; el cliente refresca internamente la
lista de usuarios conectados si necesita localizar al propietario del fichero.

## Limpieza

```bash
make clean
```
