//
// Licensed under the Apache License, Version 2.0 (the "License");
// tcp & udp server
//

package main

import (
	"flag"
	"fmt"
	"net"
	"net/http"
	"strconv"
	"strings"
)

var (
	protocol string
	addr     string
)

// tcp: http server
func handler(w http.ResponseWriter, r *http.Request) {
	fmt.Printf("from sip=[%v]\n", r.RemoteAddr)

	fmt.Fprintf(w, "hello from server: [%v]\n", addr)
}

// udp
func startUdp() {
	host, sPort, ok := strings.Cut(addr, ":")
	if !ok {
		fmt.Printf("invalid addr: %v\n", addr)
	}

	if len(host) == 0 {
		host = "0.0.0.0"
	}

	port, _ := strconv.Atoi(sPort)

	sock, err := net.ListenUDP("udp", &net.UDPAddr{
		IP:   net.ParseIP(host),
		Port: port,
	})

	if err != nil {
		fmt.Println("ListenPacket fail err", err)
		return
	}
	defer sock.Close()

	for {
		var buf [1024]byte

		n, addr, err := sock.ReadFromUDP(buf[:])
		if err != nil {
			fmt.Println("ReadFromUDP fail err", err)
			continue
		}

		payload := string(buf[:n])
		if n > 1 {
			payload = payload[:n-1]
		}

		fmt.Printf("from sip=[%s] payload=[%s]\n", addr, payload)

		msg := fmt.Sprintf("hello from server: [%v]\n", addr)
		sock.WriteToUDP([]byte(msg), addr)
	}
}

func init() {
	flag.StringVar(&addr, "a", ":8080", "listen addr")
	flag.StringVar(&protocol, "p", "tcp", "protocol")
}

func main() {
	flag.Parse()

	switch protocol {
	case "tcp":
		fmt.Printf("start tcp server: %v\n", addr)

		// http
		http.HandleFunc("/", handler)

		http.ListenAndServe(addr, nil)
	case "udp":
		fmt.Printf("start udp server: %v\n", addr)

		startUdp()
	default:
		fmt.Println("unknown protocol")
	}
}
