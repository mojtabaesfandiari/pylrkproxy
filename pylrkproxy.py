import signal
import sys
import time
from concurrent.futures.thread import ThreadPoolExecutor

from conf.config import Config
from lib.tools.common_functions import auto_create_pylrkproxy_file
auto_create_pylrkproxy_file()
Config.load_ini()
import glob
import os
import _version
from logger import logger

modules = {}


def load_modules():
    d = os.path.dirname(os.path.realpath(__file__))
    m1 = glob.glob(os.path.join(d, "lib/service/*.py"))
    module_files = m1
    # print(module_files)
    for m in module_files:
        if m.__contains__("__init__.py"):
            continue
        m = m.replace(d, "").replace(os.sep, ".").replace(".py", "")
        m = m[1:]
        _, __, mm = str(m).rpartition('.')
        try:
            p = __import__(m, fromlist=['module_name'])
            module_name = str(getattr(p, 'module_name'))
            h = getattr(p, module_name)
            modules[module_name] = h
        except Exception as e:
            logger.error('Error while loading module %s: message: %s' % (mm, e))


load_modules()


class Server:
    executor = ThreadPoolExecutor(max_workers=100)

    @staticmethod
    def run():
        for a, m in modules.items():
            if m is not None:
                future = Server.executor.submit(m.run)
                logger.debug("Loading service %s: done" % a)
            else:
                logger.error("Loading service %s: failed" % a)


def signal_handler(signal, frame):
    logger.debug('Signal SIGINT')
    if os.path.exists(Config.forward_to):
        os.remove(Config.forward_to)
    sys.exit(0)


if __name__ == "__main__":
    from lib.shell.kernel_space_module import KernelSpaceModule
    logger.info("Version: %s" % _version.__version__)
    logger.info("Starting pylrkproxy")
    signal.signal(signal.SIGINT, signal_handler)
    KernelSpaceModule.load_kernel_module()
    Server.run()

    while True:
        time.sleep(1000)
