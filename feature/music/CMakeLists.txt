if(CONFIG_FEATURE_MUSIC)
  nuttx_add_application(
    NAME
    ${CONFIG_FEATURE_MUSIC_PROGNAME}
    SRCS
    main.c ui_process.c lv_music/music_app.c lv_music/music_func.c lv_music/music_list.c lv_music/music_res.c
    STACKSIZE
    ${CONFIG_FEATURE_MUSIC_STACKSIZE}
    PRIORITY
    ${CONFIG_FEATURE_MUSIC_PRIORITY}
    MODULE
    ${CONFIG_FEATURE_MUSIC}
    DEPENDS
    lvgl)
endif()