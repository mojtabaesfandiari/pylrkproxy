import os
import subprocess

from logger import logger
from conf.config import Config
from lib.tools.common_functions import get_lrkproxy_module_directory, get_lrkproxy_module_make_file_path, \
    get_lrkproxy_module_c_file_path, get_lrkproxy_module_ko_file_path


class KernelSpaceModule:

    @classmethod
    def load_kernel_module(cls) -> None:
        if os.path.exists(get_lrkproxy_module_make_file_path()):
            if os.path.exists(get_lrkproxy_module_c_file_path()):
                proc = subprocess.Popen(
                    f"cd {get_lrkproxy_module_directory()}; make -f {get_lrkproxy_module_make_file_path()}",
                    shell=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT
                )
                stdout, stderr = proc.communicate()
                if stdout:
                    logger.debug(stdout)

                if stderr:
                    logger.error(stderr)

                logger.debug(f"{get_lrkproxy_module_ko_file_path()} created")

                if os.path.exists(get_lrkproxy_module_ko_file_path()):
                    proc = subprocess.Popen(f"/sbin/rmmod {get_lrkproxy_module_ko_file_path()}",
                                            shell=True,
                                            stdout=subprocess.PIPE,
                                            stderr=subprocess.STDOUT
                                            )
                    stdout, stderr = proc.communicate()

                    if stderr and b"Module kernel_space is not currently loaded" in stderr:
                        pass
                    elif stderr and b"Module kernel_space is not currently loaded" not in stderr:
                        logger.error(stderr)

                    if not stderr:
                        logger.debug("rmmod module of kernel space")

                    start_port = Config.start_port if Config.start_port else 20000
                    end_port = Config.end_port if Config.end_port else 30000

                    proc = subprocess.Popen(
                        f"/sbin/insmod {get_lrkproxy_module_ko_file_path()} min_port={start_port} max_port={end_port}",
                        shell=True,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT,
                    )
                    stdout, stderr = proc.communicate()

                    if stdout:
                        logger.debug(stdout)

                    if stderr:
                        logger.error(stderr)
                    logger.debug(f"insmod module of kernel space with start_port: {start_port} end_port: {end_port}")
                else:
                    logger.error(f"{get_lrkproxy_module_ko_file_path()} not found")
            else:
                logger.error(f"{get_lrkproxy_module_c_file_path()} not found")
        else:
            logger.error(f"{get_lrkproxy_module_make_file_path()} not found")
