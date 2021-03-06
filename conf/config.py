import os
import logging
import configparser


class Config:
    # Kernel
    start_port = 20000
    end_port = 30000
    current_port = 20000
    internal_ip = "192.168.10.226"
    external_ip = "192.168.10.226"  # It is under development

    # UDP socket
    socket_udp_host = "127.0.0.1"
    socket_udp_port = 8080

    # Cache
    save_call_cache = False

    # Unix socket
    forward_to = "/root/sock"

    # logger
    # log_level = logging.INFO
    log_level = logging.DEBUG
    log_to_file = True
    log_to_console = False
    log_directory = "/var/log/pylrkproxy/"
    log_file_name = "pylrkproxy.log"

    # pylrkproxy.ini file path
    pylrkproxy_ini_directory = "/etc/pylrkproxy/"
    pylrkproxy_ini_file_name = "pylrkproxy.ini"

    @classmethod
    def convert_to_logging_format(cls, s: str) -> int:
        s = str(s).lower()
        if s == "critical":
            return logging.CRITICAL
        elif s == "error":
            return logging.ERROR
        elif s == "warning":
            return logging.WARNING
        elif s == "info":
            return logging.INFO
        elif s == "debug":
            return logging.DEBUG
        elif s == "notset":
            return logging.NOTSET
        else:
            return logging.NOTSET

    @classmethod
    def load_ini(cls) -> None:
        file = None
        try:
            file = os.path.join(cls.pylrkproxy_ini_directory, cls.pylrkproxy_ini_file_name)
            if os.path.exists(file):
                variables = [i for i in dir(cls) if not callable(i) and not i.startswith("_")]
                cfg = configparser.ConfigParser()
                cfg.read(file)
                for section in cfg.sections():
                    for a, b in cfg.items(section):
                        if a in variables:
                            attr = getattr(cls, a)
                            if a == "log_level":
                                b = cls.convert_to_logging_format(b)
                            elif str(b).lower() == 'none':
                                b = None
                            elif isinstance(attr, str):
                                pass
                            elif isinstance(attr, bool):
                                # isinstance(attr, bool) and isinstance(attr, int) have conflict
                                # So check bool first to avoid inconsistency
                                if str(b).lower() == "true":
                                    b = True
                                elif str(b).lower() == "false":
                                    b = False
                                else:
                                    # logger.error('Incorrect data type in config. Variable "%s" '
                                    #              'should be boolean (True/False).' % a)
                                    pass
                            elif isinstance(attr, int):
                                if not str(b).isdigit():
                                    # logger.error('Incorrect data type in config. Variable "%s" should be integer.' % a)
                                    continue
                                b = int(b)
                            elif isinstance(attr, float):
                                try:
                                    b = float(b)
                                except ValueError as eee:
                                    # logger.error('Incorrect data type in config. Variable "%s" should be float.' % a)
                                    continue
                            elif isinstance(attr, list):
                                b = str(b).split(',') if len(str(b)) > 0 else list()
                            elif isinstance(attr, tuple):
                                b = tuple(str(b).split(',')) if len(str(b)) > 0 else tuple()
                            setattr(cls, a, b)
            else:
                # logger.debug("config file not found")
                pass
        except Exception as e:
            from logger import logger
            logger.error("unable to load %s, reason: %s" % (file, e))
