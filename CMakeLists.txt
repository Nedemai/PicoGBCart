
add_executable(PicoGBCart main.c)
target_link_libraries(PicoGBCart pico_stdlib hardware_flash)
pico_add_extra_outputs(PicoGBCart)
example_auto_set_url(PicoGBCart)
pico_set_binary_type(PicoGBCart copy_to_ram)