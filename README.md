# What?
`notibeast` is a service, sending file system notifications to its subscribers via the websocket protocol.

# Why?
It can be useful to offload processing data to a remote system. For example, we may want to transcode a video file when it gets uploaded to a fileserver. The fileserver might not have enough processing power for the task, but it is capable of notifying a remote system of such an event.

# Action!

## Explore available options

```console
./notibeast --help
```

## Play around

Run the service with:

```console
./notibeast -a 0.0.0.0 -p 8080 -m /tmp
```

Connect to the service from a client and send a subscription message with `inotify` [mask](https://www.man7.org/linux/man-pages/man7/inotify.7.html) (e.g., javascript):

```js
var socket = new WebSocket(`ws://127.0.0.1:8080`);
socket.onopen = function(event) {
  let command = {
    command: "subscribe",
    // compose a mask for all the events,
    // IN_ACCESS|IN_MODIFY|IN_ATTRIB|IN_CLOSE_WRITE|IN_CLOSE_NOWRITE|IN_OPEN|
    // IN_MOVED_FROM|IN_MOVED_TO|IN_CREATE|IN_DELETE|IN_DELETE_SELF|IN_MOVE_SELF|
    // IN_IGNORED|IN_UNMOUNT|IN_Q_OVERFLOW
    mask: 61439
  };
  socket.send(JSON.stringify(command));
}
socket.onmessage = function(event) {
  console.debug(`[message] Data received from server: ${event.data}`);
  let eventObj = JSON.parse(event.data);
  // ...
  // do whatever you want with eventObj.path, eventObj.name, eventObj.mask, eventObj.cookie
}
```

Trigger some file events on the monitored system:

```console
ls /tmp
touch /tmp/foo
mv /tmp/foo /tmp/bar
```

Receive notifications with your `socket.onmessage` handler, e.g.:

```
{"path": ".", "name": "", "mask": "IN_OPEN|IN_ISDIR", "cookie": 0}
{"path": ".", "name": "", "mask": "IN_ACCESS|IN_ISDIR", "cookie": 0}
{"path": ".", "name": "", "mask": "IN_CLOSE_NOWRITE|IN_ISDIR", "cookie": 0}

{"path": ".", "name": "foo", "mask": "IN_OPEN", "cookie": 0}
{"path": ".", "name": "foo", "mask": "IN_ATTRIB", "cookie": 0}
{"path": ".", "name": "foo", "mask": "IN_CLOSE_WRITE", "cookie": 0}

{"path": ".", "name": "foo", "mask": "IN_MOVED_FROM", "cookie": 1799}
{"path": ".", "name": "bar", "mask": "IN_MOVED_TO", "cookie": 1799}
```

## Writing a client
A more complete JS client is provided with [client.html](client.html). For ideas and inspiration on writing a C++ client visit [beast/example webpage](https://www.boost.org/doc/libs/1_76_0/libs/beast/example/websocket/client/). [Boost.json](https://www.boost.org/doc/libs/1_76_0/libs/json/doc/html/index.html) can be used for parsing received messages.

# Build

```console
mkdir build
cd build
cmake -G Ninja ..
ninja
```

## Run tests

```console
test/tests
```

## Build for ARM/Synology

It should be possible to build the service for an ARM architecture; a starting point is:

```console
BOOST_ROOT=<PATH_TO_YOUR_ARM_BOOST_BUILD> cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=../configs/toolchain-synology.cmake -DTESTS=OFF ..
```

# Run as a service

## systemd
* Modify [configs/notibeast.service](configs/notibeast.service) according to your needs
* Place the prepared file into the `/etc/systemd/system` directory
* Enable the service: `sudo systemctl enable notibeast`
* Start the service: `sudo systemctl start notibeast`

## upstart
* Modify [configs/notibeast.conf](configs/notibeast.conf) according to your needs
* Place the prepared file into the `/etc/init` directory
* Start the service: `sudo start notibeast`

# Details and internals

## File system monitoring
File system notifications are handled with [inotify](https://www.man7.org/linux/man-pages/man7/inotify.7.html). An attempt has been made to make the service monitor the file system recursively, i.e. receiving notifications for both `/tmp` and`/tmp/foo.d/` if you monitor the `/tmp` directory. However, `inotify` is [inherently racy](https://www.man7.org/linux/man-pages/man7/inotify.7.html#NOTES) and, consequently, `notibeast` is racy too.

### Why not fnotify?
[fnotify](https://man7.org/linux/man-pages/man7/fanotify.7.html) would be a better choice for monitoring a directory tree recursively. However, my Synology Diskstation returns [ENOSYS](https://man7.org/linux/man-pages/man2/fanotify_init.2.html#ERRORS) for `fanotify_init()`. It should be possible to ehance the service to support `fnotify` in the future with reasonable efforts.

## Websockets
The network part is implemented with Vinnie Falco's [Boost.beast](https://www.boost.org/doc/libs/1_76_0/libs/beast/doc/html/index.html).

# License
[Boost Software License](LICENSE_1_0.txt)
