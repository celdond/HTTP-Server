ssize_t reader(int connection_port, char *buffer, ssize_t size) {
	ssize_t i = 0;
	char terminate = '\0';
	ssize_t read = 0;

	while ((i < size - 1) && (c != '\n')) {
		read = recv(connection_port, &terminate, 1, 0);
		if (read > 0) {
			if (c == '\r') {
				read = recv(connection_port, &terminate, 1, MSG_PEEK);
				if ((read > 0) && (c == '\n')) {
					recv(connection_port, &terminate, 1, 0);
				} else {
					terminate = '\n';
				}
			}
			buffer[i] = terminate;
			i++;
		} else {
			terminate = '\n';
		}
	}
	buffer[i] = '\0';
	return i;
}
