class CallSessionCache:
    __cache = {}

    @classmethod
    def add_or_update(cls, call_id: str = None, ip: str = None, port: str = None, data: dict = None,
                      last_status: bool = None) -> None:

        if cls.__cache.get(call_id) is None:
            cls.__cache[call_id] = dict()

        if ip:
            cls.__cache[ip] = ip

        if port:
            cls.__cache[port] = port

        if data:
            cls.__cache[data] = data

        if last_status:
            cls.__cache[last_status] = last_status

    @classmethod
    def __get_field(cls, call_id: str = None, field: str = None) -> bool:
        session = cls.get_call(call_id)
        if session:
            return session.get(field)
        return False

    @classmethod
    def exists(cls, call_id: str = None) -> bool:
        return True if cls.__cache.get(call_id) else False

    @classmethod
    def get_call(cls, call_id: str = None) -> dict:
        return cls.__cache.get(call_id) if cls.exists(call_id) else None

    @classmethod
    def remove(cls, call_id: str = None) -> bool:
        if cls.exists(call_id):
            cls.__cache.pop(call_id)
            return True
        return False
