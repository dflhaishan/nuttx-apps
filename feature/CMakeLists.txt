add_subdirectory(pthread)
add_subdirectory(signal)
add_subdirectory(music)
add_subdirectory(touchpanel)
add_subdirectory(oled)
add_subdirectory(player)

nuttx_generate_kconfig(MENUDESC "Feature")