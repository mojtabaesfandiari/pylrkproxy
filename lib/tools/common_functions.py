import os
from pathlib import Path

from conf.config import Config


def get_root_project_directory() -> str:
    return str(Path(__file__).parent.parent.parent)


def get_user_space_c_file_path() -> str:
    return os.path.join(get_root_project_directory(), "lib", "c", "user_space.o", "user_space.c")


def get_user_space_o_file_path() -> str:
    return os.path.join(get_root_project_directory(), "lib", "c", "user_space.o", "user_space.o")


def get_lrkproxy_module_directory() -> str:
    return os.path.join(get_root_project_directory(), "lib", "c", "kernel_space.ko")


def get_lrkproxy_module_make_file_path() -> str:
    return os.path.join(get_root_project_directory(), "lib", "c", "kernel_space.ko", "Makefile")


def get_lrkproxy_module_c_file_path() -> str:
    return os.path.join(get_root_project_directory(), "lib", "c", "kernel_space.ko", "lrkproxy_module.c")


def get_lrkproxy_module_ko_file_path() -> str:
    return os.path.join(get_root_project_directory(), "lib", "c", "kernel_space.ko", "lrkproxy_module.ko")


def auto_create_pylrkproxy_file() -> None:
    file = os.path.join(Config.pylrkproxy_ini_directory, Config.pylrkproxy_ini_file_name)
    if not os.path.exists(file):
        content = '[kernel]\nstart_port : 20000\nend_port : 40000\ncurrent_port : 20000\n' \
                  'internal_ip : 192.168.10.10\n\n;It is under development\nexternal_ip : 192.168.10.10\n\n' \
                  '[UDP socket]\nsocket_udp_host = 0.0.0.0\nsocket_udp_port = 22333\n\n' \
                  '[Cache]\nsave_call_cache = False\n\n' \
                  '[UNIX socket]\nforward_to = /root/sock\n\n' \
                  '[logger]\nlog_to_file = True\nlog_to_console = False\n\n'
        os.makedirs(os.path.dirname(Config.pylrkproxy_ini_directory), exist_ok=True)
        with open(file, "w") as f:
            f.write(content)
