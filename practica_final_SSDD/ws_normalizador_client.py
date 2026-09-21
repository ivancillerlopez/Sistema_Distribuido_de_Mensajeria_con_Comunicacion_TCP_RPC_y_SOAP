import zeep


WSDL_NORMALIZADOR = "http://localhost:8000/?wsdl"
_cliente_ws = None


def normalizar_mensaje(mensaje):
    """Llama al servicio web y devuelve el mensaje normalizado."""
    global _cliente_ws

    try:
        # Reutilizamos el cliente SOAP para no recrearlo en cada envio.
        if _cliente_ws is None:
            _cliente_ws = zeep.Client(wsdl=WSDL_NORMALIZADOR)

        return _cliente_ws.service.normalizar(mensaje)

    except Exception as exc:
        raise RuntimeError("No se pudo normalizar el mensaje con el servicio web") from exc
