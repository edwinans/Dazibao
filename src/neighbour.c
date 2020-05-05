   for(int i = 1; i < argc; i += 2) {
        struct addrinfo hints;
        struct addrinfo *res;
        int rc;
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_DGRAM;
        rc = getaddrinfo(argv[i], argv[i + 1], &hints, &res);
        if(rc < 0) {
           /* be smart */
        }
        for(struct addrinfo *p = res; p != NULL; p = p->ai_next) {
            add_persistent_neighbour(p->ai_addr, p->ai_addrlen);
        }
        freeaddrinfo(res);
    }