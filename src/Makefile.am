# Automake file for Telegram Buffer Sender

bin_PROGRAMS = telegram_buffer_sender
telegram_buffer_sender_SOURCES = main.c main.h \
	http_parser.c http_parser.h \
	https.c https.h \
	jsmn.c jsmn.h \
	message.c message.h \
	network_parser.c network_parser.h \
	socket_interface.c socket_interface.h \
	uri_encode.c uri_encode.h

telegram_buffer_sender_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\" $(AM_CPPFLAGS)
telegram_buffer_sender_CFLAGS = $(OPENSSL_CFLAGS) $(PTHREAD_CFLAGS) $(AM_CFLAGS)
telegram_buffer_sender_LDADD = $(OPENSSL_LIBS) $(PTHREAD_LIBS)

LDADD = $(LIBINTL)
