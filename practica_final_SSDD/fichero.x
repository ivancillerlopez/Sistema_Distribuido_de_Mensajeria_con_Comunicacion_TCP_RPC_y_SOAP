struct entrada_log {
    string usuario<256>;
    string operacion<256>;
    string fichero<256>;    /* Vacío si no es sendattach */
};

program LOGPROG {
    version LOGVERS {
        void ENVIAR_LOG(struct entrada_log) = 1;
    } = 1;
} = 99;