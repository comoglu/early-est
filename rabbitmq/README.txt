To compile functions and run tests:

1) Install rabbitmq from http://www.rabbitmq.com
2) Check and edit environment variables in the Makefile in this directory for your platform
3) Run:
        make clean
        make
4) Start rabbitmq-server (from http://www.rabbitmq.com), e.g.
        /usr/local/sbin/rabbitmq-server &
            or
        /usr/local/opt/rabbitmq/sbin/rabbitmq-server &
5) In a console, start listener, e.g.
        amqp_listen_test localhost 5672 / guest guest test test
6) In another console, send a string, which should be echoed in the listener, e.g.
        amqp_sendstring_test localhost 5672  / guest guest test topic test "Hello World"


7) In the end:
        /usr/local/sbin/rabbitmqctl stop
            or
        /usr/local/opt/rabbitmq/sbin/rabbitmqctl stop
