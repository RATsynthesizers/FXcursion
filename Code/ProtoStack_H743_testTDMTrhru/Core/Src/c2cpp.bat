powershell -Command "(gc main.c)  -replace 'MX_SAI1_Init', '//' | Out-File -encoding ASCII main.c"
powershell -Command "(gc main.c)  -replace 'MX_SAI2_Init', 'MX_SAI2_Init();MX_SAI1_Init();//' | Out-File -encoding ASCII main.c"
ren *main.c *main.cpp