# Template Do File For Altium Designer -> Specctra Autorouter
# Altium Limited
# 22-Apr-2015
#
unit mil
bestsave on D:\YandexDisk\Stud\Prog\Synth\_Altium\_ProtoStack\ProtoStack_H750VBT\topor\ProtoStack_H750VBT.bst
status_file D:\YandexDisk\Stud\Prog\Synth\_Altium\_ProtoStack\ProtoStack_H750VBT\topor\ProtoStack_H750VBT.sts
grid smart (wire 1) (via 1)
smart_route
critic

#enable the spread and miter features if you have the DFM option
#spread
#miter

# If you have the DFM module use spread and miter instead of the following. 
# Comment these lines out
Center
Recorner Diagonal 2000 2000 2000
Recorner Diagonal 1000 1000 1000
Recorner Diagonal 500 500 500
Recorner Diagonal 250 250 250
Recorner Diagonal 125 125 125
Recorner Diagonal 100 100 100
Recorner Diagonal 50 50 50
Recorner Diagonal 25 25 25
Recorner Diagonal 10 10 10
# Stop commenting here if you have the DFM module


write  routes      D:\YandexDisk\Stud\Prog\Synth\_Altium\_ProtoStack\ProtoStack_H750VBT\topor\ProtoStack_H750VBT.rte
write  wires       D:\YandexDisk\Stud\Prog\Synth\_Altium\_ProtoStack\ProtoStack_H750VBT\topor\ProtoStack_H750VBT.w
report conflicts   D:\YandexDisk\Stud\Prog\Synth\_Altium\_ProtoStack\ProtoStack_H750VBT\topor\ProtoStack_H750VBT.rcf
report corners     D:\YandexDisk\Stud\Prog\Synth\_Altium\_ProtoStack\ProtoStack_H750VBT\topor\ProtoStack_H750VBT.rcn
report rules       D:\YandexDisk\Stud\Prog\Synth\_Altium\_ProtoStack\ProtoStack_H750VBT\topor\ProtoStack_H750VBT.rrl
report status      D:\YandexDisk\Stud\Prog\Synth\_Altium\_ProtoStack\ProtoStack_H750VBT\topor\ProtoStack_H750VBT.rst
report unconnect   D:\YandexDisk\Stud\Prog\Synth\_Altium\_ProtoStack\ProtoStack_H750VBT\topor\ProtoStack_H750VBT.ruc
report vias        D:\YandexDisk\Stud\Prog\Synth\_Altium\_ProtoStack\ProtoStack_H750VBT\topor\ProtoStack_H750VBT.rva
quit
