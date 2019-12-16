This is a serial test applet under linux.

Build:
    
    make
Clean Build:
    
    make clean
  
Run:
  
    #Loopback test
    ./serail_tool /dev/ttyS0 -rsavo -b 115200 -m 232
    
    #Receive data
    ./serail_tool /dev/ttyS0 -rsavo -b 115200
  
    #Send data
    ./serail_tool /dev/ttyS0 -rsavo -b 115200
