import os
import threading
import time
import psutil
import math

from logger import logger

module_name = "MonitoringService"


class MonitoringService:

    @classmethod
    def run(cls) -> None:
        threading.Thread(target=cls.worker_system_information, args=(), daemon=True).start()

    @classmethod
    def worker_system_information(cls) -> None:
        try:
            while True:
                process = psutil.Process(os.getpid())
                logger.debug(f"Ram used percentage: {cls.get_data_ram()}, "
                             f"Cpu used percentage: {cls.get_data_cpu()},"
                             f"Memory RSS: {cls.convert_size(process.memory_info().rss)}, "
                             f"Memory VMS: {cls.convert_size(process.memory_info().vms)}, "
                             f"Full info Memory: {process.memory_full_info()}, "
                             f"Active Thread: {threading.active_count()}, "
                             f"Memory used python: {psutil.Process(os.getpid()).memory_info().rss / 1024 ** 2} MB")
                time.sleep(2 * 60)
        except Exception as e:
            logger.error(e)
            time.sleep(60)

    @classmethod
    def get_data_ram(cls) -> int:
        try:
            total = psutil.virtual_memory().total
            used = psutil.virtual_memory().used
            percent = round(used / total * 100, 1)

            return percent
        except Exception as e:
            logger.error(e)
            return 0

    @classmethod
    def get_data_cpu(cls) -> int:
        try:
            return psutil.cpu_percent(interval=1)
        except Exception as e:
            logger.error(e)
            return 0

    @classmethod
    def convert_size(cls, size_bytes: int) -> str:
        if size_bytes == 0:
            return "0B"
        size_name = ("B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB")
        i = int(math.floor(math.log(size_bytes, 1024)))
        p = math.pow(1024, i)
        s = round(size_bytes / p, 2)
        return "%s %s" % (s, size_name[i])
