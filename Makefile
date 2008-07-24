WARNINGS = -Wall -Wextra
#LIBRARIES = -lIL -ljpeg -lpng -lcv -lcxcore -lhighgui -framework OpenGL -framework GLUT -lpthread
LIBRARIES = -lIL -ljpeg -lpng -lcv -lcxcore -lhighgui -lGL -lGLEW -lglut -lpthread
#LIBRARIES = -static -lIL -ljpeg -lpng -lcv -lcxcore -lhighgui -lpthread
#LIBRARIES = -lIL -ljpeg -lpthread
#2 sets of configuration are provided, one for debugging (the first) and one for speed on the system that I am using (you may need to change this!)

#Debug
#SYS = 
#DEBUG = -DDEBUG
#OPTIMAL = ${DEBUG}
#GLOBALOPTS = -g3 -pedantic -pg ${WARNINGS}

#Speed
SYS = -msse2 -march=nocona
#DEBUG = -DNDEBUG
OPTIMAL = -O3 -pipe -mfpmath=sse -funroll-loops ${DEBUG} ${SYS}
GLOBALOPTS = -fomit-frame-pointer -pedantic ${WARNINGS}

CC = g++
SRCDIR = src
OBJDIR = obj

all:  NonParametric.o Image8.o ListLoader.o configReader.o fitness.o particleSwarm.o devil.o bgSubMain.o particleSwarmBGSub.o particleSwarmBGSubMultiSeq.o uniformParameterSelection.o

NonParametric.o:  ${SRCDIR}/backgroundSubtraction/NonParametric.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/backgroundSubtraction/NonParametric.cpp

NonParametricGPU.o:  ${SRCDIR}/backgroundSubtraction/NonParametricGPU.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/backgroundSubtraction/NonParametricGPU.cpp

Image8.o:  ${SRCDIR}/imageClass/Image8.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/imageClass/Image8.cpp

ListLoader.o:  ${SRCDIR}/imageProvider/ListLoader.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/imageProvider/ListLoader.cpp

ChipLoader.o:  ${SRCDIR}/imageProvider/ChipLoader.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/imageProvider/ChipLoader.cpp

OpenCVReader.o:  ${SRCDIR}/imageProvider/OpenCVReader.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/imageProvider/OpenCVReader.cpp

UDPClient.o:  ${SRCDIR}/imageProvider/UDPClient.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/imageProvider/UDPClient.cpp

configReader.o:  ${SRCDIR}/configTools/configReader.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/configTools/configReader.cpp

fitness.o:  ${SRCDIR}/PSO/fitness.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/PSO/fitness.cpp

particleSwarm.o:  ${SRCDIR}/PSO/particleSwarm.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/PSO/particleSwarm.cpp

devil.o:  ${SRCDIR}/imageFormat/devil.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/imageFormat/devil.cpp

bgSubMain.o: ${SRCDIR}/bgSubMain.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/bgSubMain.cpp

bgSubMainCV.o: ${SRCDIR}/bgSubMainCV.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/bgSubMainCV.cpp

bgSubMainGPU.o: ${SRCDIR}/bgSubMainGPU.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/bgSubMainGPU.cpp

bgSubMainUDP.o: ${SRCDIR}/bgSubMainUDP.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/bgSubMainUDP.cpp

InputDetection.o: ${SRCDIR}/InputDetection.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/InputDetection.cpp

connectedComponents.o: ${SRCDIR}/imageTools/connectedComponents.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/imageTools/connectedComponents.cpp

font.o: ${SRCDIR}/imageTools/font.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/imageTools/font.cpp

imageTools.o: ${SRCDIR}/imageTools/imageTools.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/imageTools/imageTools.cpp

ChangeDetector.o: ${SRCDIR}/imageTools/ChangeDetector.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/imageTools/ChangeDetector.cpp

binaryImageTools.o: ${SRCDIR}/imageTools/binaryImageTools.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/imageTools/binaryImageTools.cpp

particleSwarmBGSub.o: ${SRCDIR}/particleSwarmBGSub.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/particleSwarmBGSub.cpp

particleSwarmBGSubMultiSeq.o: ${SRCDIR}/particleSwarmBGSubMultiSeq.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/particleSwarmBGSubMultiSeq.cpp

uniformParameterSelection.o: ${SRCDIR}/uniformParameterSelection.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/uniformParameterSelection.cpp
		
GPUModule.o: ${SRCDIR}/GPU/GPUModule.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/GPU/GPUModule.cpp
		
GPUModuleNP.o: ${SRCDIR}/GPU/GPUModuleNP.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/GPU/GPUModuleNP.cpp

GPUModuleNPMAC.o: ${SRCDIR}/GPU/GPUModuleNP.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -DMAC -c ${SRCDIR}/GPU/GPUModuleNP.cpp

SVMClassifier.o: ${SRCDIR}/objectClassifier/SVMClassifier.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/objectClassifier/SVMClassifier.cpp

featureCalculator.o: ${SRCDIR}/objectClassifier/featureCalculator.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/objectClassifier/featureCalculator.cpp

svmModelCreate.o: ${SRCDIR}/svmModelCreate.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/svmModelCreate.cpp

chipper.o: ${SRCDIR}/chipper.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/chipper.cpp

#Copies the object file from the libsvm dir
svm.o: ${SRCDIR}/objectClassifier/libsvm-2.83/svm.o
		cp ${SRCDIR}/objectClassifier/libsvm-2.83/svm.o ./

svmtest.o: ${SRCDIR}/svmtest.cpp
		${CC} ${CPPFLAGS} ${GLOBALOPTS} ${OPTIMAL} -c ${SRCDIR}/svmtest.cpp

inputDetection:  Image8.o ListLoader.o configReader.o devil.o connectedComponents.o InputDetection.o binaryImageTools.o OpenCVReader.o
		swig -c++ -python  -outdir ./ -o InputDetection_wrap.cxx ./${SRCDIR}/InputDetection.h
		g++ -c InputDetection_wrap.cxx -I/Library/Frameworks/Python.framework/Versions/2.5/include/python2.5
		${CC} -bundle -flat_namespace -undefined suppress ${LDFLAGS} ${GLOBALOPTS} ${OPTIMAL} -lIL -ljpeg -lpthread -lcv -lcxcore -lhighgui Image8.o ListLoader.o configReader.o devil.o connectedComponents.o InputDetection.o binaryImageTools.o InputDetection_wrap.o OpenCVReader.o -o _InputDetection.so

inputDetectionLIN:  Image8.o ListLoader.o configReader.o devil.o connectedComponents.o InputDetection.o binaryImageTools.o NonParametric.o OpenCVReader.o font.o imageTools.o ChangeDetector.o svm.o imageTools.o SVMClassifier.o
		swig -c++ -python  -outdir ./ -o InputDetection_wrap.cxx ./${SRCDIR}/InputDetection.h
		g++ -c InputDetection_wrap.cxx -I/usr/include/python2.5
		${CC} -shared ${LDFLAGS} ${GLOBALOPTS} ${OPTIMAL} ${LIBRARIES} Image8.o ListLoader.o configReader.o devil.o connectedComponents.o NonParametric.o InputDetection.o binaryImageTools.o ChangeDetector.o InputDetection_wrap.o OpenCVReader.o font.o imageTools.o svm.o SVMClassifier.o -o _InputDetection.so

svmCreate:  svmModelCreate.o Image8.o ChipLoader.o devil.o configReader.o svm.o imageTools.o SVMClassifier.o featureCalculator.o
		${CC} ${LDFLAGS} ${GLOBALOPTS} ${OPTIMAL} ${LIBRARIES} svmModelCreate.o SVMClassifier.o Image8.o svm.o imageTools.o ChipLoader.o devil.o configReader.o featureCalculator.o -o svmCreate

chipper:  chipper.o Image8.o devil.o imageTools.o
		${CC} ${LDFLAGS} ${GLOBALOPTS} ${OPTIMAL} ${LIBRARIES} chipper.o Image8.o imageTools.o devil.o -o chipper

bgSub:  Image8.o ListLoader.o configReader.o devil.o bgSubMain.o uniformParameterSelection.o connectedComponents.o NonParametric.o
		${CC} ${LDFLAGS} ${GLOBALOPTS} ${OPTIMAL} ${LIBRARIES} Image8.o ListLoader.o configReader.o devil.o bgSubMain.o NonParametric.o connectedComponents.o -o bgSub
		
bgSubCV:  Image8.o ListLoader.o configReader.o devil.o bgSubMainCV.o uniformParameterSelection.o connectedComponents.o GPUModuleNP.o NonParametric.o NonParametricGPU.o OpenCVReader.o
				${CC} ${LDFLAGS} ${GLOBALOPTS} ${OPTIMAL} ${LIBRARIES} Image8.o ListLoader.o configReader.o devil.o bgSubMainCV.o NonParametric.o connectedComponents.o OpenCVReader.o GPUModuleNP.o NonParametricGPU.o -o bgSub

bgSubCVMAC:  Image8.o ListLoader.o configReader.o devil.o bgSubMainCV.o uniformParameterSelection.o connectedComponents.o GPUModuleNPMAC.o NonParametric.o NonParametricGPU.o OpenCVReader.o
								${CC} ${LDFLAGS} ${GLOBALOPTS} ${OPTIMAL} ${LIBRARIES} Image8.o ListLoader.o configReader.o devil.o bgSubMainCV.o NonParametric.o connectedComponents.o OpenCVReader.o GPUModuleNP.o NonParametricGPU.o -o bgSub

#NOTE <-----------  GPUModule.o has been removed from below
bgSubGPU:  Image8.o ListLoader.o configReader.o devil.o bgSubMainGPU.o NonParametricGPU.o connectedComponents.o GPUModuleNP.o
		${CC} ${LDFLAGS} ${GLOBALOPTS} ${OPTIMAL} -framework OpenGL -framework GLUT -lIL -ljpeg Image8.o ListLoader.o configReader.o devil.o bgSubMainGPU.o connectedComponents.o NonParametricGPU.o GPUModuleNP.o -o bgSubGPU

bgSubGPULIN:  Image8.o ListLoader.o configReader.o devil.o bgSubMainGPU.o NonParametricGPU.o connectedComponents.o GPUModuleNP.o
				${CC} ${LDFLAGS} ${GLOBALOPTS} ${OPTIMAL} ${LIBRARIES} Image8.o ListLoader.o configReader.o devil.o bgSubMainGPU.o connectedComponents.o NonParametricGPU.o GPUModuleNP.o -o bgSubGPU
		
bgSubUDP:  Image8.o ListLoader.o configReader.o devil.o UDPClient.o bgSubMainUDP.o connectedComponents.o
		${CC} ${LDFLAGS} ${GLOBALOPTS} ${OPTIMAL} ${LIBRARIES} Image8.o ListLoader.o configReader.o devil.o UDPClient.o bgSubMainUDP.o connectedComponents.o -o bgSubUDP

pso:  Image8.o imageTools.o ListLoader.o configReader.o fitness.o binaryImageTools.o particleSwarm.o devil.o particleSwarmBGSub.o uniformParameterSelection.o connectedComponents.o NonParametric.o NonParametricGPU.o GPUModuleNP.o
				${CC} ${LDFLAGS} ${GLOBALOPTS} ${OPTIMAL} ${LIBRARIES} Image8.o imageTools.o binaryImageTools.o ListLoader.o configReader.o fitness.o particleSwarm.o devil.o particleSwarmBGSub.o connectedComponents.o NonParametric.o NonParametricGPU.o GPUModuleNP.o -o pso

svm:  SVMClassifier.o svm.o svmtest.o
				${CC} ${LDFLAGS} ${GLOBALOPTS} ${OPTIMAL} ${LIBRARIES} SVMClassifier.o svm.o svmtest.o -o svm

psomulti:  Image8.o ListLoader.o configReader.o fitness.o particleSwarm.o devil.o particleSwarmBGSubMultiSeq.o connectedComponents.o uniformParameterSelection.o NonParametricGPU.o GPUModuleNP.o
				${CC} ${LDFLAGS} ${GLOBALOPTS} ${OPTIMAL} ${LIBRARIES} Image8.o ListLoader.o configReader.o fitness.o particleSwarm.o devil.o particleSwarmBGSubMultiSeq.o connectedComponents.o NonParametricGPU.o GPUModuleNP.o -o psomulti

uniform:  Image8.o ListLoader.o configReader.o fitness.o particleSwarm.o devil.o uniformParameterSelection.o connectedComponents.o
				${CC} ${LDFLAGS} ${GLOBALOPTS} ${OPTIMAL} ${LIBRARIES} Image8.o ListLoader.o configReader.o fitness.o particleSwarm.o devil.o uniformParameterSelection.o connectedComponents.o -o uniform
		
clean:
	rm -f *.so *.o svmCreate bgSub bgSubGPU bgSubUDP pso psomulti uniform

#Made to remove more things than clean, including results, etc.  (Different to avoid mistakes)
cleanall:
	rm -f *.so *.o bgSub bgSubGPU bgSubUDP pso psomulti uniform *.log *~
