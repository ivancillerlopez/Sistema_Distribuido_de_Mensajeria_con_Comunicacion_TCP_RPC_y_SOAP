import logging

from wsgiref.simple_server import make_server
from spyne import Application, ServiceBase, Unicode, rpc
from spyne.protocol.soap import Soap11
from spyne.server.wsgi import WsgiApplication


class Normalizador(ServiceBase):
    """Servicio SOAP que normaliza el texto enviado por los clientes."""

    @rpc(Unicode, _returns=Unicode)
    def normalizar(ctx, mensaje):
        """Elimina espacios repetidos y devuelve palabras separadas por un solo espacio."""
        if mensaje is None:
            return ""

        # Convierte textos como "hola     mundo" en "hola mundo".
        return " ".join(mensaje.split())


# Aplicacion SOAP publicada por el servidor WSGI.
application = Application(
    services=[Normalizador],
    tns='http://ssdd.uc3m.es/normalizador',
    in_protocol=Soap11(validator='lxml'),
    out_protocol=Soap11()
)

application = WsgiApplication(application)


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO)

    logging.info("Servicio web escuchando en http://127.0.0.1:8000")
    logging.info("WSDL disponible en http://localhost:8000/?wsdl")

    # Servidor local usado por los clientes antes de enviar mensajes.
    server = make_server('127.0.0.1', 8000, application)
    server.serve_forever()
