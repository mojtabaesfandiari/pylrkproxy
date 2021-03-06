import queue
import threading
import time

from conf.config import Config
from lib.cache.call_status_request_cache import CallStatusRequest
from lib.service.unix_socket_client_service import UnixSocketClientService
from logger import logger

module_name = "SessionControllerService"


class SessionControllerService:
    add_request_data_queue = queue.Queue()
    delete_request_data_queue = queue.Queue()

    @classmethod
    def run(cls) -> None:
        threading.Thread(target=cls.consumer_add_request, args=(), daemon=True).start()
        threading.Thread(target=cls.consumer_delete_request, args=(), daemon=True).start()

    @classmethod
    def consumer_add_request(cls) -> None:
        try:
            local_requests = list()
            last_get_time = time.time()
            while True:
                try:
                    req = cls.add_request_data_queue.get()
                    cls.add_request_data_queue.task_done()

                    if len(local_requests) == 0:
                        last_get_time = time.time()

                    local_requests.append(req)

                    # if queue is empty,
                    # waiting method for check again
                    if cls.add_request_data_queue.qsize() == 0:
                        time.sleep(0.1)

                    if time.time() - last_get_time < 1 and cls.add_request_data_queue.qsize() > 0:
                        continue

                    if cls.add_request_data_queue.qsize() > 100:
                        logger.warning(f"queue data_request have many waited request len:{cls.add_request_data_queue.qsize()}")

                    for req in local_requests:
                        cls.data_request_handler(req)

                    # clear list after send data
                    local_requests = list()

                except queue.Empty as e:
                    logger.debug(f"data_request is Empty ( {e} )")
        except Exception as e:
            logger.error(e)
            time.sleep(3)

    @classmethod
    def consumer_delete_request(cls) -> None:
        try:
            local_requests = list()
            last_get_time = time.time()
            while True:
                try:
                    req = cls.delete_request_data_queue.get()
                    cls.delete_request_data_queue.task_done()

                    if len(local_requests) == 0:
                        last_get_time = time.time()

                    local_requests.append(req)

                    # if queue is empty,
                    # waiting method for check again
                    if cls.delete_request_data_queue.qsize() == 0:
                        time.sleep(0.1)

                    if time.time() - last_get_time < 1 and cls.delete_request_data_queue.qsize() > 0:
                        continue

                    if cls.delete_request_data_queue.qsize() > 100:
                        logger.warning(f"queue data_request have many waited request len:{cls.delete_request_data_queue.qsize()}")

                    for req in local_requests:
                        cls.data_request_handler(req)

                    # clear list after send data
                    local_requests = list()

                except queue.Empty as e:
                    logger.debug(f"data_request is Empty ( {e} )")
        except Exception as e:
            logger.error(e)
            time.sleep(3)

    @classmethod
    def data_request_handler(cls, data: bytes = None) -> None:
        # TODO save call session
        # logger.debug("SessionControllerService ((( Request ))): %s" % data)

        # ------------- Set fake data -------------------
        data = data.decode("utf-8")
        data = data.split(" ")
        # if not data[2]:
        #     logger.debug("----------------- SET FAKE IP FOR FIRST -----------------")
        #     logger.debug("----------------- SET FAKE IP FOR FIRST -----------------")
        #     logger.debug("----------------- SET FAKE IP FOR FIRST -----------------")
        #     logger.debug("----------------- SET FAKE IP FOR FIRST -----------------")
        #     logger.debug("----------------- SET FAKE IP FOR FIRST -----------------")
        #     data[2] = "172.1.1.171"
        #
        # if not data[5]:
        #     logger.debug("----------------- SET FAKE IP FOR LAST -----------------")
        #     logger.debug("----------------- SET FAKE IP FOR LAST -----------------")
        #     logger.debug("----------------- SET FAKE IP FOR LAST -----------------")
        #     logger.debug("----------------- SET FAKE IP FOR LAST -----------------")
        #     logger.debug("----------------- SET FAKE IP FOR LAST -----------------")
        #     logger.debug("----------------- SET FAKE IP FOR LAST -----------------")
        #     data[5] = "192.168.10.249"
        data = " ".join(data)
        data = data.encode("utf-8")
        # -----------------------------------------------
        request_id, command = cls.get_request_id_and_command(data)
        call_id = cls.get_call_id(data)
        # logger.debug("call_id: %s, date: %s" % (call_id, data))

        if Config.save_call_cache:
            CallStatusRequest.add(call_id, request_id, add=request_id)

        UnixSocketClientService.data_request_queue.put(data)
        # if not CallStatusRequest.exists_call_id(call_id):
        #     CallStatusRequest.add(call_id, request_id, add=request_id)
        #     UnixSocketClientService.data_request_queue.put(data)

    @classmethod
    def get_call_id(cls, data: bytes = None) -> str:
        data = data.decode("utf-8").strip().replace("  ", " ").split(" ")
        return data[11] if len(data) >= 11 else None

    @classmethod
    def get_request_id_and_command(cls, data: bytes = None) -> (str, str):
        data = data.decode("utf-8").strip().replace("  ", " ").split(" ")
        return (data[0], data[1]) if len(data) >= 2 else (None, None)
