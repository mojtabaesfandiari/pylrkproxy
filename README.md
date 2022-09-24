# pylrkproxy - Light RTP Kernel Proxy

#### Installation


You should download the source package locally and run with command:

Extract project:

```
tar -xvf pylrkproxy-master.tar.gz -C /usr/src/
mv /usr/src/pylrkproxy-master /usr/src/pylrkproxy
```

Install psutil:

```
apt update
apt install python3-pip
pip3 install psutil
```

Install linux headers:

```
uname -r
apt install linux-headers-$(uname -r)
```

Run project with command:

```
python3 /usr/src/pylrkproxy/pylrkproxy.py
```


----
#### Dependencies

pylrkproxy supports `Python3.6+` and test on `Debian GNU/Linux 10 (buster)`

----

#### Structure code

In this project, requests are sent to pylrkproxy from Kamailio on the UDP socket and then sent to the kernel space via user space.

Each request contains the following commands:

- P (PING)
    ```
    request_id P
    ```
    By sending this request from Kamailio, he sends pylrkproxy in response to `PONG` to inform Kamailio that the was ready.
    <br/>

- G (CONFIG)
   ```
   request_id G
   ```
    By sending this request from Kamailio, he sends pylrkproxy in response to this information `start_port`, `end_port`, `current_port`, `internal_ip` and  `external_ip`.
    The config file is in the following path: `/etc/pylrkproxy/pylrkproxy.ini`.
    <br/> 

- S (DATA)
    ```
    request_id S src_ip dst_ip s_nat_ip d_nat_ip src_port dst_port s_nat_port d_nat_port timeout call_id
    ```
    By sending this request from Kamailio to pylrkproxy, the data is sent to the kernel space

----

#### Config files

The config file is in ``/etc/pylrkproxy/pylrkproxy.ini`` path.

These settings are available both in the `/usr/src/pylrkproxy/conf/config.py` path inside the source project and the `/etc/pylrkproxy/pylrkproxy.ini` path.
When the project is executed, the variables inside the `pylrkproxy.ini` file replace the variables inside the `config.py`, so the priority is with the `pylrkproxy.ini` file.

To apply the new settings, just change the variables in the `pylrkproxy.ini` file and run the project again.

Default config:
```
[kernel]
start_port = 20000
end_port = 30000
current_port = 20000
internal_ip = 192.168.10.226

;It is under development
external_ip = "192.168.10.226"

[UDP socket]
socket_udp_host = 127.0.0.1
socket_udp_port = 8080

[Cache]
save_call_cache = False

[UNIX socket]
forward_to = /root/sock

[logger]
log_to_file = True
log_to_console = False
```
----

#### Log files
Use the following command to check lrkproxy_module logs:

```
dmesg -hs
```


Use the following command to check user_space and pylrkproxy logs:

```
tail -n 100 -f /var/log/pylrkproxy/pylrkproxy.log
```

```
tail -n 100 -f /var/log/pylrkproxy/user_space.log
```