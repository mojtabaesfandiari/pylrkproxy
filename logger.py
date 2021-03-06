import logging.handlers
import os
from conf.config import Config

if not os.path.exists(Config.log_directory):
    os.makedirs(Config.log_directory)


class MyLogger(logging.Logger):

    def makeRecord(self, name, level, fn, lno, msg, args, exc_info,
                   func=None, extra=None, sinfo=None):
        rv = super(MyLogger, self).makeRecord(name, level, fn, lno, msg, args, exc_info,
                                              func, extra, sinfo)
        if rv.thread:
            rv.thread = int(str(rv.thread)[-5:])
        return rv


logging.setLoggerClass(MyLogger)
# logging.basicConfig()

LOG_FILENAME = Config.log_directory + Config.log_file_name
logger = logging.getLogger("Sniffer %s" % Config.log_file_name)
logger.propagate = False
logger.setLevel(Config.log_level)

if Config.log_to_file is True:
    handler = logging.handlers.TimedRotatingFileHandler(
        LOG_FILENAME, when='D', interval=1, backupCount=10, encoding='utf-8')
    formatter = logging.Formatter('%(asctime)s [%(thread)s] %(levelname)s  %(module)s line:%(lineno)d %(message)s')
    # formatter = logging.Formatter('%(asctime)s   %(levelname)s   %(module)s line:%(lineno)d %(message)s')
    handler.setFormatter(formatter)
    logger.addHandler(handler)

if Config.log_to_console is True:
    # create console handler and set level to debug
    handler = logging.StreamHandler()
    handler.setLevel(logging.DEBUG)
    # create formatter
    formatter = logging.Formatter('%(asctime)s [%(thread)s] %(levelname)s  %(module)s line:%(lineno)d %(message)s')
    # add formatter to ch
    handler.setFormatter(formatter)
    logger.addHandler(handler)
