# Dazibao
A Dazibao in C Network programming style.

## config
```
$cd src
open the core.c file
change init_neighbours(my_pair, JCH_HOST, UDP_DEF_PORT)
to init_neighbours(my_pair, [BOOT_HOST], [BOOT_PORT])
```

to change the node_data:
in function init_pair(...), just change data[] string.


## build the project

```
$ cd src
$ make core       # compile the core file
./core            # run in normal mode
./core		  # run in deubg mode (console logs)	
```
