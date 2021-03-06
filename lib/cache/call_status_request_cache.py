class CallStatusRequest:
    __cache = {}

    @classmethod
    def add(cls, call_id: str = None, request_id: str = None, add: str = None, delete: str = None) -> None:

        if not cls.__cache.get(call_id):
            cls.__cache[call_id] = {
                "request_ids": [],
                "add": "",
                "delete": ""
            }

        if request_id:
            cls.__cache.get(call_id, {}).get("request_ids", []).append(request_id)

        if add:
            cls.__cache.get(call_id, {})["add"] = add
            cls.set_add_call()

        if delete:
            cls.__cache.get(call_id, {})["delete"] = add
            cls.set_delete_call()

    @classmethod
    def set_add_call(cls) -> None:
        if not cls.__cache.get("add_calls"):
            cls.__cache["add_calls"] = 1
        else:
            cls.__cache["add_calls"] += 1

    @classmethod
    def set_delete_call(cls) -> None:
        if not cls.__cache.get("delete_calls"):
            cls.__cache["delete_calls"] = 1
        else:
            cls.__cache["delete_calls"] += 1

    @classmethod
    def set_add_request(cls) -> None:
        if not cls.__cache.get("requests"):
            cls.__cache["requests"] = 1
        else:
            cls.__cache["requests"] += 1

    @classmethod
    def insert_remove_requests(cls) -> None:
        if not cls.__cache.get("remove_requests"):
            cls.__cache["remove_requests"] = 1
        else:
            cls.__cache["remove_requests"] += 1

    @classmethod
    def insert_update_requests(cls) -> None:
        if not cls.__cache.get("update_requests"):
            cls.__cache["update_requests"] = 1
        else:
            cls.__cache["update_requests"] += 1

    @classmethod
    def insert_received_requests(cls) -> None:
        if not cls.__cache.get("received_requests"):
            cls.__cache["received_requests"] = 1
        else:
            cls.__cache["received_requests"] += 1

    @classmethod
    def insert_call_id(cls, call_id=None) -> None:
        if not cls.__cache.get("call_id"):
            cls.__cache["call_id"] = []
        cls.__cache.get("call_id").append(call_id)

    @classmethod
    def exists_call_id(cls, call_id=None):
        return True if cls.__cache.get(call_id) else False

    @classmethod
    def get_all(cls) -> dict:
        return cls.__cache
