if(CONFIG_FEATURE_PLAYER)
  nuttx_add_application(
    NAME
    ${CONFIG_FEATURE_PLAYER_PROGNAME}
    SRCS
    main.c player_process.c player.c player_mp3.c
    STACKSIZE
    ${CONFIG_FEATURE_PLAYER_STACKSIZE}
    PRIORITY
    ${CONFIG_FEATURE_PLAYER_PRIORITY}
    MODULE
    ${CONFIG_FEATURE_PLAYER})
endif()