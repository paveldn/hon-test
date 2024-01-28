# Haier's hOn protocol tester
This is an ESPhome-based configuration with a custom UART component. The main purpose of this software is to be able to keep communication with the hOn appliance through serial protocol.

## Short description

The custom UART component accepts report and alarm messages and has the functionality to send custom packets.
![image](https://github.com/paveldn/hon-test/assets/11540146/c79ab0e5-4648-4639-9340-35f8e2b7591e)

The demo configuration has implementation to send some well-known types of commands (get status, get device version, etc.), also there is an API service implementation that can be used to send any packet (should be used together with Home Assistant)
