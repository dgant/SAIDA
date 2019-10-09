*******************************************************************************
        SAIDA (Samsung Artificial Intelligence & Data Analytics)
*******************************************************************************

Environment Setup :
- Visual Studio 2013
- Uses Windows environment variable BWAPI_DIR which points to BWAPI 4.1.2
  (ex : BWAPI_DIR = C:\StarCraft\bwlibrary\BWAPI412)
To Compile :
- Open src/SAIDA.sln in VS2013
- Select Release_Server_DLL mode
- Build the SAIDA project
- Output will go to src/Release_Server_DLL/SAIDA/SAIDA.dll

Tournament Setup :
- Copy SAIDA.dll to the tournament SAIDA/AI folder

References :
UAlberta Bot (Thanks to Author : David Churchill)
BWEM (Map Analyzer, Thanks to Author : Igor Dimitrijevic)

Brief Strategy : 
SAIDA is able to change battle strategies and various combinations of units dynamically 
according to the enemy strategy to get higher probabilities for winning the game. 
Also it has high performance in gathering resources. 
Finally it does powerful attack at appropriate time that is decided by our ML model.

###########################################################
SAIDA team of Samsung SDS from South Korea
Team Members : Changhyeon Bae(Leader), Daehun Jun, Iljoo Yoon, Junseung Lee, 
               Hyunjae Lee, Hyunjin Choi, Uk Jo, Yonghyun Jeong
Leader Contact : charlie.bae@samsung.com, mathto@naver.com
About Samsung SDS : http://www.samsungsds.com/global/en/index.html
