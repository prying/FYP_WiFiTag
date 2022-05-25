idf_component_register(
    SRCS 
        "main.c"
        "beaconApp.c"
        "beaconBLE.c"
        "WiFi.c"
        "http.c"
        "databaseApp.c"
    INCLUDE_DIRS "."
    EMBED_TXTFILES ca.pem
)
