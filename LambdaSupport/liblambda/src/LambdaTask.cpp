/*
 * (c) Copyright 2014-2017 DESY, Yuelong Yu <yuelong.yu@desy.de>
 *
 * This file is part of FS-DS detector library.
 *
 * This software is free: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************
 *     Author: Yuelong Yu <yuelong.yu@desy.de>
 */

#include "LambdaTask.h"
#include "LambdaModule.h"
#include "ImageDecoder.h"
#include "DistortionCorrector.h"
#include "LambdaSysImpl.h"

namespace DetLambdaNS
{   
    //////////////////////////////////////////////////
    ///LambdaTask
    //////////////////////////////////////////////////
    LambdaTask::LambdaTask()
    {
        LOG_TRACE(__FUNCTION__);
    }
        
    LambdaTask::LambdaTask(string _strTaskName, Enum_priority _Epriority, int32 _nID,
                           LambdaSysImpl* _objSys,NetworkInterface* _objNetInt,
                           MemPool<char>* _objMemPoolRaw,MemPool<int16>* _objMemPoolDecoded12,
                           boost::mutex* _bstMtx,vector<int16> _vCurrentChip,
                           vector<int32> _vNIndex,vector<int32> _vNNominator)
        :m_objSys(_objSys),
         m_objNetInterface(_objNetInt),
         m_objMemPoolRaw(_objMemPoolRaw),
         m_objMemPoolDecodedShort(_objMemPoolDecoded12),
         m_boostMtx(_bstMtx),
         m_vCurrentChip(_vCurrentChip),
         m_vNIndex(_vNIndex),
         m_vNNominator(_vNNominator)
    {
        LOG_TRACE(__FUNCTION__);
        
        m_strTaskName = _strTaskName;
        m_enumPriority = _Epriority;
        m_nID = _nID;
        m_enumTargetPriority = &m_enumPriority; // Default behaviour will ignore priority
        
        m_nRawImageSize = m_objMemPoolRaw->GetElementSize();
        m_nDecodedImageSize = m_objMemPoolDecodedShort->GetElementSize();
    }

    LambdaTask::LambdaTask(string _strTaskName, Enum_priority _Epriority, int32 _nID,
                           LambdaSysImpl* _objSys,NetworkInterface* _objNetInt,
                           MemPool<char>* _objMemPoolRaw,MemPool<int32>* _objMemPoolDecoded24,
                           boost::mutex* _bstMtx,vector<int16> _vCurrentChip,
                           vector<int32> _vNIndex,vector<int32> _vNNominator)
        :m_objSys(_objSys),
         m_objNetInterface(_objNetInt),
         m_objMemPoolRaw(_objMemPoolRaw),
         m_objMemPoolDecodedInt(_objMemPoolDecoded24),
         m_boostMtx(_bstMtx),
         m_vCurrentChip(_vCurrentChip),
         m_vNIndex(_vNIndex),
         m_vNNominator(_vNNominator)
    {
        LOG_TRACE(__FUNCTION__);
        
        m_strTaskName = _strTaskName;
        m_enumPriority = _Epriority;
        m_nID = _nID;
        
        m_nRawImageSize = m_objMemPoolRaw->GetElementSize();
        m_nDecodedImageSize = m_objMemPoolDecodedInt->GetElementSize();
    }

    LambdaTask::LambdaTask(string _strTaskName, Enum_priority _Epriority, int32 _nID,
                           LambdaSysImpl* _objSys,NetworkInterface* _objNetInt,
                           MemPool<char>* _objMemPoolRaw,MemPool<char>* _objMemPoolCompressed,
                           boost::mutex* _bstMtx,vector<int16> _vCurrentChip,
                           vector<int32> _vNIndex,vector<int32> _vNNominator,int32 _nDistortedImageSize)
        :m_objSys(_objSys),
         m_objNetInterface(_objNetInt),
         m_objMemPoolRaw(_objMemPoolRaw),
         m_objMemPoolCompressed(_objMemPoolCompressed),
         m_boostMtx(_bstMtx),
         m_vCurrentChip(_vCurrentChip),
         m_vNIndex(_vNIndex),
         m_vNNominator(_vNNominator),
         m_nDecodedImageSize(_nDistortedImageSize)
    {
        LOG_TRACE(__FUNCTION__);
        
        m_strTaskName = _strTaskName;
        m_enumPriority = _Epriority;
        m_nID = _nID;
        
        m_nRawImageSize = m_objMemPoolRaw->GetElementSize();
        //m_nDecodedImageSize = m_objMemPoolCompressed->GetElementSize();
    }
    
    LambdaTask::~LambdaTask()
    {
        LOG_TRACE(__FUNCTION__);
    }


    void LambdaTask::SetCompressedBuffer(MemPool<char>* objMemPoolCompressed)
    {
        LOG_TRACE(__FUNCTION__);
        m_objMemPoolCompressed = objMemPoolCompressed;
    }
    
    void LambdaTask::DoTaskAction()
    {
        LOG_TRACE(__FUNCTION__);

        if(m_strTaskName == "StartAcquisition")
            DoAcquisition();
        if(m_strTaskName == "StartAcquisitionTCP")
            DoAcquisitionTCP();
        else if(m_strTaskName == "DecodeImage")       
            DoDecodeImage();
        else if(m_strTaskName == "MultiLink")
            DoAcquisitionWithMultiLink();
        else if(m_strTaskName == "MonitorTask")
            DoMonitorListener();
        m_objSys->GetState(); // When task exits, useful to check state.

    }
    
    void LambdaTask::DoMonitorListener()
    {
        LOG_TRACE(__FUNCTION__);

        boost::unique_lock<boost::mutex> lock(*m_boostMtx);
        int32 lRequestedImgNo = m_objSys->GetNImages();
        bool bIsStart = m_objSys->GetAcquisitionStart();
        if((m_objSys->GetReadoutModeCode() == OPERATION_MODE_24)
           || (m_objSys->GetReadoutModeCode() == OPERATION_MODE_2x12))
            lRequestedImgNo*=2;
        lock.unlock();
        //cout<<"listner thread starts"<<m_nID<<endl;
        
        while(true)
        {   
            //while(m_objMemPoolRaw->GetTotalReceivedPackets()<(lRequestedImgNo*133*0.8))
            //    usleep(1000);
            usleep(200);
            
            if(m_objMemPoolRaw->GetTotalReceivedFrames()>=lRequestedImgNo || (!bIsStart))
                break;
	     
            m_objMemPoolRaw->IsImageFinished();
	     
            lock.lock();
            bIsStart = m_objSys->GetAcquisitionStart();
            lock.unlock();
        }
        //cout<<"listner thread exits"<<m_nID<<endl;
    }
    
    void LambdaTask::DoAcquisitionWithMultiLink()
    {    
        LOG_TRACE(__FUNCTION__);

        char* ptrchPacket = new char[UDP_PACKET_SIZE_NORMAL];    
        szt nPacketSize = UDP_PACKET_SIZE_NORMAL;
        int16 shErrorCode = 0;
        int32 lFrameNo = 0;
        int16 shPacketSequenceNo = 0;
        int32 nPos = 0;
        m_objMemPoolRaw->AddTaskFrame(m_nID,-1);
        boost::unique_lock<boost::mutex> lock(*m_boostMtx);
        int32 lRequestedImgNo = m_objSys->GetNImages();
        bool bIsStart = m_objSys->GetAcquisitionStart();
        if((m_objSys->GetReadoutModeCode() == OPERATION_MODE_24)
           || (m_objSys->GetReadoutModeCode() == OPERATION_MODE_2x12))
            lRequestedImgNo*=2;
        lock.unlock();
        //int32 nCurrentFrame = -1;

        int32 nCount = 200;
        int32 nNoDataTimes = 0;

	bool bRollover = false; // Flag for detecting rollover
	int32 nMaxFrameNo = pow(2,24)-1; // Rollover occurs at end of 24-bit counter
	//cout << "Rollover test value" << nMaxFrameNo;
        

        //cout<<"listner thread starts"<<m_nID<<endl;
        while(true)
        {
            if(m_objMemPoolRaw->GetTotalReceivedFrames()>=lRequestedImgNo)
            {		
                break;
            }
            else if(!bIsStart)
            {
                //m_objNetInterface->ReceiveData(ptrchTmpImg,m_nRawImageSize);
                while(true)
                {
                    shErrorCode = m_objNetInterface->ReceivePacket(ptrchPacket,nPacketSize);
                    if(shErrorCode==-1)
                    {
                        nNoDataTimes++;
                        //cout << "Timeouttest ";
                        if(nNoDataTimes==nCount)
                            break;
                    }
                    else 
                    {
                        nNoDataTimes = 0;
                        //cout << "Extrapacket ";
                    }
                }
                break;
            }

            shErrorCode = m_objNetInterface->ReceivePacket(ptrchPacket,nPacketSize);
            
            if(shErrorCode!=-1)
            {
                lFrameNo = (uchar)ptrchPacket[5]
                    +(uchar)ptrchPacket[4]*256
                    +(uchar)ptrchPacket[3]*256*256;

		if(lFrameNo == nMaxFrameNo)
		{
		    bRollover = true;
		    //cout << "Rollover detected";
		}
		if(bRollover && (lFrameNo < nMaxFrameNo))
		{
		    lFrameNo += (nMaxFrameNo+1); // In special case of rollover, subsequent images should have their number increased
		}
		               
                shPacketSequenceNo = (uchar)ptrchPacket[2];
                //if(shPacketSequenceNo == 1)
                //    nPos = 0;
                //else
                nPos = (UDP_PACKET_SIZE_NORMAL-UDP_EXTRA_BYTES)*(shPacketSequenceNo-1);
                m_objMemPoolRaw->SetPacket(ptrchPacket,nPos,nPacketSize,
                                           lFrameNo,shErrorCode,m_nID,shPacketSequenceNo);
            }
            else
            {
                usleep(10); // Avoid hitting socket and mutexes too much if no packets available 
            }
            
            lock.lock();
            bIsStart = m_objSys->GetAcquisitionStart();
            lock.unlock();
        }
        
        lock.lock();
        m_objSys->SetAcquisitionStart(false);
        // m_objSys->SetState(ON);
        lock.unlock();
        //cout<<"listner thread exits"<<m_nID<<endl;
        delete ptrchPacket;
    }
  
    void LambdaTask::DoAcquisition()
    {
        LOG_TRACE(__FUNCTION__);

        char* ptrchPacket = new char[UDP_PACKET_SIZE_NORMAL];    
        szt nPacketSize = UDP_PACKET_SIZE_NORMAL;
        int16 shErrorCode = 0;
        
        char* ptrchTmpImg = new char[m_nRawImageSize];
        int16 nErrorCode = 0;
        int32 lFrameNo = 0;

        szt nCurrentPacketSize = 0;

        int nPacketCode = 0;
        int nReceivedData = 0;
        int nRetVal = -1;
        int nFrameLength = m_nRawImageSize;
        int nBytesLeft = nFrameLength;
        int nExcessBytes = 0;

        boost::unique_lock<boost::mutex> lock(*m_boostMtx);
        int32 lRequestedImgNo = m_objSys->GetNImages();
        if((m_objSys->GetReadoutModeCode() == OPERATION_MODE_24)
           || (m_objSys->GetReadoutModeCode() == OPERATION_MODE_2x12))
            lRequestedImgNo*=2;
        bool bIsStart = m_objSys->GetAcquisitionStart();
        //bool bIsFull = m_objMemPoolRaw->IsFull();
        lock.unlock();

        int32 nCount = 200;
        int32 nNoDataTimes = 0;
        
        while(true)
        {
            //Test conditions for exiting acquisition loop
            if(lFrameNo>=lRequestedImgNo)
            {                
                break;
            }
            
            lock.lock();
            bIsStart = m_objSys->GetAcquisitionStart();
            lock.unlock();
            
            if(!bIsStart)
            {
                //If acq stopped, empty any remaining packets from buffer
                while(true)
                {
                    nPacketCode = m_objNetInterface->ReceivePacket(ptrchPacket,nCurrentPacketSize);
                    if(nPacketCode==-1)
                    {
                        nNoDataTimes++;
                        if(nNoDataTimes>=nCount)
                        {
                            nNoDataTimes = 0;
                            break;
                        }
                    }
                    else
                    {
                        nNoDataTimes = 0;
                    }
                }
                break;
            }
          

            // NOTE - Sergej's UDP implementation uses a fixed packet size
            // Just ditch excess bytes
            // Error recovery more complicated - avoid this for now.
            if(nExcessBytes > 0)
            {
                nExcessBytes = 0;
            }

            while(true)
            {
                nPacketCode = m_objNetInterface->ReceivePacket(ptrchPacket,nCurrentPacketSize);
                if(nPacketCode==-1)
                {
                    nNoDataTimes++;
                    if(nNoDataTimes>=nCount)
                    {
                        nNoDataTimes = 0;
                        break;
                    }
                }
                else
                {
                    // We have data 
                    nNoDataTimes = 0;
                    if(nCurrentPacketSize > nBytesLeft)
                    {
                        //Bytes from next image are present in buffer - need to take note of this
                        std::copy(ptrchPacket,ptrchPacket+nBytesLeft,ptrchTmpImg+nReceivedData);
                        nReceivedData += nBytesLeft;
                        nExcessBytes = nCurrentPacketSize-nBytesLeft;
                        nBytesLeft = 0;
                    }
                    else
                    {
                        std::copy(ptrchPacket,ptrchPacket+nCurrentPacketSize,ptrchTmpImg+nReceivedData);
                        nReceivedData += nCurrentPacketSize;
                        nBytesLeft = nBytesLeft-(nCurrentPacketSize);
                    }
                }

                if(nBytesLeft==0)
                {
                    //first byte of the image should be 0xa0
                    unsigned char uchByte = ptrchTmpImg[0];
                    if(uchByte!=0xa0)
                    {        
                        LOG_STREAM(__FUNCTION__,ERROR,"Image data is wrong!");
                        nErrorCode = 2;
                    }
                    else nErrorCode = 0;
                    lFrameNo++; // Make first frame no 1, for consistency with multilink approach
                    m_objMemPoolRaw->SetImage(ptrchTmpImg,lFrameNo,nErrorCode);          
                    LOG_INFOS(("Arrived Frame No is:"+to_string(static_cast<long long>(lFrameNo))));
                    // Reset some variables
                    nReceivedData = 0;
                    nBytesLeft = nFrameLength;
                    nErrorCode = 0;
                    break;
                }
            }

        }///end of loop

        lock.lock();
        m_objSys->SetAcquisitionStart(false);
        //m_objSys->SetState(ON);
        lock.unlock();

        delete ptrchPacket;
        delete ptrchTmpImg;
        LOG_INFOS("Do acquisition thread exits");
    }

    void LambdaTask::DoAcquisitionTCP()
    {
        //Acquisition with TCP
        //Firstly, need opportunity to break if acq cancelled
        //Secondly, at high data rates packets from consecutive
        //images may be combined - need to handle this carefully
        LOG_TRACE(__FUNCTION__);
        int32 nTCPPacketSize = 1500;
        char* ptrchPacket = new char[nTCPPacketSize];

        size_t nCurrentPacketSize;
	
        char* ptrchTmpImg = new char[m_nRawImageSize];
        int32 lFrameNo = 0;

        int32 nErrorCode = 0;
        int32 nPacketCode = 0;
        int32 nReceivedData = 0;
//        int32 nRetVal = -1;
        int32 nFrameLength = m_nRawImageSize;
        int32 nBytesLeft = nFrameLength;
        int32 nExcessBytes = 0;

        boost::unique_lock<boost::mutex> lock(*m_boostMtx);
        int32 lRequestedImgNo = m_objSys->GetNImages();
        if((m_objSys->GetReadoutModeCode() == OPERATION_MODE_24)
           || (m_objSys->GetReadoutModeCode() == OPERATION_MODE_2x12))
            lRequestedImgNo*=2;
        bool bIsStart = m_objSys->GetAcquisitionStart();
        // bool bIsFull = m_objMemPoolRaw->IsFull();
        lock.unlock();

        int32 nCount = 200;
        int32 nNoDataTimes = 0;

        while(true) // Loop over images
        {
            //Test conditions for exiting acquisition loop
            if(lFrameNo>=lRequestedImgNo)
            {		
                break;
            }
	    
            lock.lock();
            bIsStart = m_objSys->GetAcquisitionStart();
            lock.unlock();
	    
            if(!bIsStart)
            {
                //If acq stopped, empty any remaining packets from buffer
                while(true)
                {
                    nPacketCode = m_objNetInterface->ReceivePacket(ptrchPacket,nCurrentPacketSize);
                    if(nPacketCode==-1)
                    {
                        nNoDataTimes++;
                        if(nNoDataTimes>=nCount)
                        {
                            nNoDataTimes = 0;
                            break;
                        }
                    }
                    else
                    {
                        nNoDataTimes = 0;
                    }
                }
                break;
            }

            // Special case where single packet contains data from 2 images
            if(nExcessBytes > 0) 
            {
                std::copy(ptrchPacket+nCurrentPacketSize-nExcessBytes,
                          ptrchPacket+nCurrentPacketSize,ptrchTmpImg+nReceivedData);
                nReceivedData += nExcessBytes;
                nBytesLeft = nBytesLeft-nExcessBytes;
                nExcessBytes = 0;
            }
            //NOTE that in theory we might exit this loop mid-image
            while(true)
            {
                nPacketCode = m_objNetInterface->ReceivePacket(ptrchPacket,nCurrentPacketSize);
                if(nPacketCode==-1)
                {
                    nNoDataTimes++;
                    if(nNoDataTimes>=nCount)
                    {
                        nNoDataTimes = 0;
                        break;
                    }
                }
                else
                {
                    // We have data 
                    nNoDataTimes = 0;
                    if(nCurrentPacketSize > static_cast<szt>(nBytesLeft))
                    {
                        //Bytes from next image are present in buffer - need to take note of this
                        std::copy(ptrchPacket,ptrchPacket+nBytesLeft,ptrchTmpImg+nReceivedData);
                        nReceivedData += nBytesLeft;
                        nExcessBytes = nCurrentPacketSize-nBytesLeft;
                        nBytesLeft = 0;
                    }
                    else
                    {
                        std::copy(ptrchPacket,ptrchPacket+nCurrentPacketSize,
                                  ptrchTmpImg+nReceivedData);
                        nReceivedData += nCurrentPacketSize;
                        nBytesLeft = nBytesLeft-(nCurrentPacketSize);
                    }
                }

                if(nBytesLeft==0)
                {
                    //first byte of the image should be 0xa0
                    uchar uchByte = ptrchTmpImg[0];
                    if(uchByte!=0xa0)
                    {	
                        LOG_STREAM(__FUNCTION__,ERROR,"Image data is wrong!");
                        nErrorCode = 2;
                    }
                    else nErrorCode = 0;
                    
                    // Make first frame no 1, for consistency with multilink approach
                    lFrameNo++; 
                    m_objMemPoolRaw->SetImage(ptrchTmpImg,lFrameNo,nErrorCode);	  
                    LOG_INFOS(("Arrived Frame No is:"+to_string(lFrameNo)));
                    // Reset some variables
                    nReceivedData = 0;
                    nBytesLeft = nFrameLength;
                    nErrorCode = 0;
                    break;
                }
            }

        }///end of loop

        lock.lock();
        m_objSys->SetAcquisitionStart(false);
        //m_objSys->SetState(ON);
        lock.unlock();

        delete ptrchPacket;
        delete ptrchTmpImg;
        LOG_INFOS("Do acquisition thread exits");
    }
    
    void LambdaTask::DoDecodeImage()
    {
        LOG_TRACE(__FUNCTION__);
       
        char* ptrchTmpImg;
        char* ptrchTmpImg1;
        ptrchTmpImg = 0; // Null pointer initially
        ptrchTmpImg1 = 0;
        ImageDecoder* objDecoder = new ImageDecoder(m_vCurrentChip);
        ImageDecoder* objDecoder1 = new ImageDecoder(m_vCurrentChip);

        int32 nXtemp;
        int32 nYtemp;
        objDecoder->GetDecodedImageSize(nXtemp, nYtemp);
	
        int32 nImageSizeBeforeDistCorr = nXtemp * nYtemp;

        int16* ptrshDecodedImg;
        int16* ptrshDecodedImg1;
        int32* ptrnDecodedImg = new int32[nImageSizeBeforeDistCorr];
        int32* ptrnFinishedImg;

        DistortionCorrector<int16> *objDC
            = new DistortionCorrector<int16>(m_vNIndex,m_vNNominator,((int)pow(2,12)-1));
        DistortionCorrector<int32> *objDC1
            = new DistortionCorrector<int32>(m_vNIndex,m_vNNominator,((int)pow(2,24)-1));

        CompressionContext* objCompressionContext
            = new CompressionContext(unique_ptr<CompressionInterface>(new CompressionZlib()));


        // Compression-related vectors, if needed
        // Try creating on the stack, for now; can change to heap if needed
        vector<uchar> vuchData;
        vector<uchar> vuchDstData;
         

        while(true)
        {
            usleep(10);

            boost::unique_lock<boost::mutex> lock(*m_boostMtx);
            bool bVal = m_objSys->SysExit();
            Enum_readout_mode enumOperationMode = m_objSys->GetReadoutModeCode();
            int32 nDistortionCorr = m_objSys->GetDistortionCorrecttionMethod();
            //int32 nMethod = m_objSys->GetCompressionMethod();
            bool bCompressionEnabled;
            int32 nCompLevel;
            m_objSys->GetCompressionEnabled(bCompressionEnabled,nCompLevel);
            lock.unlock();

            if(bVal)
                break;

            if(*m_enumTargetPriority > m_enumPriority) // If priority lower, throttle back task
            {
                usleep(10000);
                continue;
            }

            //continuousReadWrite or 2 x 12 bit
            if((enumOperationMode == OPERATION_MODE_12)
               || (enumOperationMode == OPERATION_MODE_2x12))
            {
                if(m_objMemPoolRaw->GetStoredImageNumbers() == 0)
                {
                    usleep(50);
                    continue;
                }	  

                if(!bCompressionEnabled)
                {    
                    if(m_objMemPoolDecodedShort->IsFull())
                    {
                        usleep(50);
                        continue;
                    }
                }
                else
                {
                    if(m_objMemPoolCompressed->IsFull())
                    {
                        usleep(50);
                        continue;
                    }
                }
                
	
                int16 shErrCode = 0;
                int32 lFrameNo = 0;
                int32 nDataLength;
                
                if(m_objMemPoolRaw->GetImage(ptrchTmpImg,lFrameNo,shErrCode,nDataLength))
                {
                    objDecoder->SetRawImage(ptrchTmpImg);
                    ptrshDecodedImg = objDecoder->RunDecodingImg();


                    if(nDistortionCorr == 1)
                    {
                        //distortion correction
                        int16* pShImgOut = objDC->RunDistortCorrect(ptrshDecodedImg);
                        ptrshDecodedImg = pShImgOut;
                    }

                    if(!bCompressionEnabled)
                    {
                        
                        if(m_objMemPoolDecodedShort->GetFirstFrameNo() == -1)
                        {
                            int32 lFristFrame = m_objMemPoolRaw->GetFirstFrameNo();               
                            //if lFristFrame is -1, means it is single link version
                            if(lFristFrame!=-1)
                                m_objMemPoolDecodedShort->SetFirstFrameNo(lFristFrame);
                        }
                    }
                    else
                    {    
                        if(m_objMemPoolCompressed->GetFirstFrameNo() == -1)
                        {
                            int32 lFristFrame = m_objMemPoolRaw->GetFirstFrameNo();               
                            //if lFristFrame is -1, means it is single link version
                            if(lFristFrame!=-1)
                                m_objMemPoolCompressed->SetFirstFrameNo(lFristFrame);
                        }
                    }
                    
                    //cout<<lFrameNo<<":is taken for decoding"<<endl;

                    //Avoid repeated iteration here!

                    
                    //usleep(20);

                    lock.lock();
                    

                    //upate live data
                    m_objSys->SetCurrentImage(ptrshDecodedImg,lFrameNo,shErrCode);
                    lock.unlock();


                    if(bCompressionEnabled)
                    {
                        int32 nTotalSize = m_nDecodedImageSize*sizeof(int16);

                        vuchData.resize(nTotalSize);
                        memmove(&vuchData[0],ptrshDecodedImg,nTotalSize);

                        if(!objCompressionContext->CompressData(vuchData,vuchDstData,nCompLevel))
                            cout<<objCompressionContext->GetErrorMessage()<<endl;

                        
                        
                        char* ptrchData = reinterpret_cast<char*>(&vuchDstData[0]);
                        // Loop where we try to stick image into buffer
                        while(true)
                        {
                            bool bIsAcq = m_objSys->GetAcquisitionStop();
                            if(bIsAcq)
                                break;

                            if((m_objMemPoolCompressed->SetImage(ptrchData
                            
                                                                 ,lFrameNo,shErrCode
                                                                 ,true
                                                                 ,vuchDstData.size())) == true)
                            {
                                break;
                            }
                            usleep(20); // Wait for next attempt
                        }
                    }
                    else    
                    {
                                   
                        while(true)
                        {
                            bool bIsAcq = m_objSys->GetAcquisitionStop();
                            if(bIsAcq)
                                break;
                            if(m_objMemPoolDecodedShort->SetImage(
                                   ptrshDecodedImg,lFrameNo,shErrCode,true) == true)
                            {
			         
                                break;
                            }
                            usleep(20); // Wait for next attempt
                        }
                    }

                }
                
            }
            else if(enumOperationMode == OPERATION_MODE_24)
            {
                if(m_objMemPoolRaw->GetStoredImageNumbers() < 2)
                {
                    usleep(50);
                    continue;
                }	  


                if(!bCompressionEnabled)
                {    
                    if(m_objMemPoolDecodedInt->IsFull())
                    {
                        usleep(50);
                        continue;
                    }
                }
                else
                {
                    if(m_objMemPoolCompressed->IsFull())
                    {
                        usleep(50);
                        continue;
                    }
                }
                int16 shErrCode = 0;
                int32 lFrameNo = 0;
                int16 shErrCode1 = 0;
                int32 lFrameNo1 = 0;
                
                if(m_objMemPoolRaw->Get2Image(ptrchTmpImg,lFrameNo,shErrCode,
                                              ptrchTmpImg1,lFrameNo1,shErrCode1))
                {
                
                    objDecoder->SetRawImage(ptrchTmpImg);
                    ptrshDecodedImg = objDecoder->RunDecodingImg();

                    objDecoder1->SetRawImage(ptrchTmpImg1);
                    ptrshDecodedImg1 = objDecoder1->RunDecodingImg();

                    for(int32 i=0;i<nImageSizeBeforeDistCorr;i++)
                        ptrnDecodedImg[i] = ((int32)ptrshDecodedImg[i])
                            +(((int32)ptrshDecodedImg1[i])*4096);
                    
                    lFrameNo = lFrameNo1/2;
                    shErrCode = shErrCode<=shErrCode1?shErrCode:shErrCode1;
                    ptrnFinishedImg = ptrnDecodedImg;
                
                    if(nDistortionCorr == 1)
                    {
                        //distortion correction
                        int32* pNImgOut = objDC1->RunDistortCorrect(ptrnDecodedImg);
                        ptrnFinishedImg = pNImgOut;
                    }

                    if(!bCompressionEnabled)
                    {
                        
                        if(m_objMemPoolDecodedInt->GetFirstFrameNo() == -1)
                        {
                            int32 lFristFrame = m_objMemPoolRaw->GetFirstFrameNo();
                
                            //if lFristFrame is -1, means it is single link version
                            if(lFristFrame!=-1)
                                m_objMemPoolDecodedInt->SetFirstFrameNo((lFristFrame+1)/2);
                        }
                    }
                    else
                    {    
                        if(m_objMemPoolCompressed->GetFirstFrameNo() == -1)
                        {
                            int32 lFristFrame = m_objMemPoolRaw->GetFirstFrameNo();
                
                            //if lFristFrame is -1, means it is single link version
                            if(lFristFrame!=-1)
                                m_objMemPoolCompressed->SetFirstFrameNo((lFristFrame+1)/2);
                        }
                    }
                    //usleep(20);

                    lock.lock();
                    //upate live data
                    m_objSys->SetCurrentImage(ptrnFinishedImg,lFrameNo,shErrCode);
                    lock.unlock();
                
                    if(bCompressionEnabled)
                    {
                        int32 nTotalSize = m_nDecodedImageSize*sizeof(int32);

                        vuchData.resize(nTotalSize);
                        memmove(&vuchData[0],ptrnFinishedImg,nTotalSize);
			 

                        if(!objCompressionContext->CompressData(vuchData,vuchDstData,nCompLevel))
                            cout<<objCompressionContext->GetErrorMessage()<<endl;
                        char* ptrchData = reinterpret_cast<char*>(&vuchDstData[0]);

                        // Loop where we try to stick image into buffer
                        while(true)
                        {
                            bool bIsAcq = m_objSys->GetAcquisitionStop();
                            if(bIsAcq)
                                break;

                            if((m_objMemPoolCompressed->SetImage(
                                    ptrchData,lFrameNo,shErrCode,true,vuchDstData.size())) == true)
                            {
                                break;
                            }
                            usleep(20); // Wait for next attempt
                        }
                    }
                    else
                    {
                        while(true)
                        {
                            bool bIsAcq = m_objSys->GetAcquisitionStop();
                            if(bIsAcq)
                                break;
                            
                            if(m_objMemPoolDecodedInt->SetImage(
                                   ptrnFinishedImg,lFrameNo,shErrCode,true) == true)
                                break;
                            usleep(20); // Wait for next attempt
                        }
                    }
                }   
            }
        }///end loop
        
        delete objDecoder;
        delete objDecoder1;
        delete[] ptrnDecodedImg;

        delete objDC;

        delete objDC1;

        delete objCompressionContext;

        LOG_INFOS("Do decoding thread exits");	
    }
}
