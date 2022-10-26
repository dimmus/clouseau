<div align="center">

# Clouseau

</div>

Clouseau is a UI inspection tool aimed to debug EFL applications.

Requirements:

* efl (1.20 +)

## Compiling:

    $ mkdir build
    $ cd build
    $ cmake -DCMAKE_INSTALL_PREFIX=/your_install_path ..
    $ make
    $ sudo make install

## How to use it?

1. Run efl_debugd on the machine where the program to debug has to run
2. Run your program
3. Run `clouseau_client -l / -r [port]`
4. Choose the extension you want to use
5. Choose your application

clouseau_client can run in three modes:
- locally: it connects to the local daemon (efl_debugd)
- remotely: you have to establish a connection (SSH...) and to supply the port to use to Clouseau
- offline: this is for the case you want to visualize a snapshot saved earlier
