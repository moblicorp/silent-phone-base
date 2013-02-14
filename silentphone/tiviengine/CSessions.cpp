/*
Created by Janis Narbuts
Copyright © 2004-2012 Tivi LTD,www.tiviphone.com. All rights reserved.
Copyright © 2012-2013, Silent Circle, LLC.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Any redistribution, use, or modification is done solely for personal 
      benefit and not for any commercial purpose or for monetary gain
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name Silent Circle nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL SILENT CIRCLE, LLC BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "CSessions.h"
#include "../encrypt/md5/md5.h"

int CSessionsBase::iRandomCounter=0;

const char CPhSesions::szTagChars[80]="0123456789qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNMabcdef";

int isVideoCall(int iCallID){
   CSesBase * s=g_getSesByCallID(iCallID);
   if(!s)return 0;
   CTSesMediaBase *m=s? s->mBase:NULL;
   return (m && m->getMediaType()&CTSesMediaBase::eVideo)?1:0;
}

int getMediaInfo(int iCallID, const char *key, char *p, int iMax){

   CSesBase * s=g_getSesByCallID(iCallID);
   if(!s)return 0;
   //if zrtp. //Take it from CTMediaIDS
   CTSesMediaBase *m=s?s->mBase:NULL;
   if(m){
      return m->getInfo(key,p,iMax);
   }
   return -2;
}

static int getMediaInfo(CSesBase *s, const char *key, char *p, int iMax){
   
   if(!s)return 0;
   //if zrtp. //Take it from CTMediaIDS
   CTSesMediaBase *m=s?s->mBase:NULL;
   if(m){
      return m->getInfo(key,p,iMax);
   }
   return -2;
}

int getCallInfo(int iCallID, const char *key, char *p, int iMax){
   CSesBase * s=g_getSesByCallID(iCallID);
   if(!s)return 0;
   
   if(strncmp(key,"media.",6)==0)return getMediaInfo(s,key+6,p,iMax);
   if(strncmp(key,"zrtp.",5)==0)return getMediaInfo(s,key+5,p,iMax);//?? why +5? rem and test
  // if(strncmp(key,"zrtp.",5)==0){CTZRTP *z=findSessionZRTP(s);if(z)return z->getInfoX(key+5, p, iMax);else return 0;}//test
   
   
   return s->getInfo(key, p, iMax);;
}

int CPhSesions::getCallType(int iCallID){
   if(!iCallID)return 0;

   //will never crash, the sessions are always existing
   CSesBase *s = findSessionByID(iCallID);
   int ret=0;
   if(s){
      ret=1;
      if(s->iMediaType&CTSesMediaBase::eVideo)ret|=4;
   }
   
   return ret;
}


void CPhSesions::createCallId(CSesBase *spSes, int iIsReg)
{
   #define OUR_CALL_ID_LEN     (16)

   int i,y;
   unsigned int res[4];


   char *szTag=spSes->sSIPMsg.rawDataBuffer+spSes->sSIPMsg.uiOffset;
   spSes->sSIPMsg.dstrCallID.strVal=szTag;
   spSes->sSIPMsg.dstrCallID.uiLen=OUR_CALL_ID_LEN;
   spSes->sSIPMsg.uiOffset+=spSes->sSIPMsg.dstrCallID.uiLen;
   
   int sp=(int)spSes;
   
   CTMd5 md5;
   CTMd5 md5pwd;
   md5pwd.update((unsigned char *)"salt",4);
   md5pwd.update((unsigned char *)&p_cfg.user.pwd[0], strlen(p_cfg.user.pwd));
   unsigned int pwd_res = md5pwd.final();
   
   
   int iDevIDLen=0;
#if defined(ANDROID_NDK) || defined(__APPLE__)  
   const char *t_getDevID(int &l);
   //TODO save random token when app starts 1st time and use it
   const char *p = t_getDevID(iDevIDLen);
   if(iDevIDLen>0)
      md5.update((unsigned char *)p,iDevIDLen);
#endif   
   
   if(iDevIDLen<=0)
      md5.update((unsigned char *)&uiRegistarationCallIdRandom,sizeof(uiRegistarationCallIdRandom));
   
   md5.update((unsigned char *)&p_cfg.user.un[0] , strlen(p_cfg.user.un));
   ////if user has changed password then call-id should be different 
   md5.update((unsigned char *)&pwd_res, 1);//must be 1 byte
   md5.update((unsigned char *)&p_cfg.user.nr[0] , strlen(p_cfg.user.nr));
   if(!iIsReg){
      md5.update((unsigned char *)&iRandomCounter, 4);
      md5.update((unsigned char *)&extAddr.ip, 4);
      md5.update((unsigned char *)&ipBinded, 4);
      md5.update((unsigned char *)&sp, 4);
      md5.update((unsigned char *)&p_cfg.iNet, 4);
      md5.update((unsigned char *)&uiGT, sizeof(uiGT));
   }
   md5.final((unsigned char*)&res[0]);
   
   
   
   iRandomCounter+=(int)uiGT;
   iRandomCounter+=(int)res[3];


   for(i=0;i<OUR_CALL_ID_LEN/3;i++)
   { 
      y=res[0]&63;//TAG_CHAR_CNT;
      res[0]>>=6;
      szTag[i]=szTagChars[y];
   }
   for(;i<OUR_CALL_ID_LEN/3*2;i++)
   {
      y=res[1]&63;//%TAG_CHAR_CNT;
      szTag[i]=szTagChars[y];
      res[1]>>=6;
   }
   for(;i<OUR_CALL_ID_LEN;i++)
   {
      y=res[2]&63;//%TAG_CHAR_CNT;
      szTag[i]=szTagChars[y];
      res[2]>>=6;
   }

}

void CPhSesions::createTag(CSesBase *spSes)
{

   unsigned int i,x,y;
#define OUR_TAG_LEN     4

   iRandomCounter+=70000007;
   x=(int)spSes;
   
   x*=(int)getTickCount();
   x+=iRandomCounter;

   for(i=0;i<OUR_TAG_LEN;i++)
   {
      y=x&63;
      spSes->str16Tag.strVal[i]=szTagChars[y];
      x>>=6;
   }
   spSes->str16Tag.uiLen=OUR_TAG_LEN;
}

void CPhSesions::handleSessions()
{
   CSesBase *spSes=NULL;;
   int i;

   LOCK_MUTEX_SES
   for (i=0;i<iMaxSesions;i++)
   {
      if(pSessionArray[i].cs.iBusy==FALSE)continue;
      spSes=&pSessionArray[i];
      if(spSes->mBase && 
         (spSes->cs.iCallStat==CALL_STAT::EInit || spSes->cs.iCallStat==CALL_STAT::EOk)
         )
      {
         spSes->mBase->onTimer();
      }


      if(spSes->uiClearMemAt && spSes->uiClearMemAt<uiGT)
      {
         onKillSes(spSes,0,NULL,spSes->cs.iSendS);

      }
      else
      {
         if(spSes->cs.iWaitS && (spSes->cs.iSendS || spSes->cs.iCallStat==CALL_STAT::ESendError || spSes->cs.iWaitS==METHOD_ACK))
         {
            if(spSes->sSendTo.iRetransmitions<0)
            {
               onKillSes(spSes, -1, NULL, spSes->cs.iSendS);
            }
            else if(spSes->sSendTo.uiNextRetransmit>0 && spSes->sSendTo.uiNextRetransmit<uiGT)
            {
               if(spSes->sSendTo.iRetransmitions>0)
                  sendSip(sockSip,spSes);
               else
                  spSes->sSendTo.updateRetransmit(uiGT);
            }
         }
      }

   }
   UNLOCK_MUTEX_SES
}


CTAudioOutBase *findAOByMB(CTSesMediaBase *mb);

CSesBase *CPhSesions::findSessionByZRTP(CTZRTP *z){
   CTSesMediaBase *mb;
   CSesBase *spSes;
   
   if(!z)return NULL;
   //z->pSes==spSes
   
   for(int i=0;i<iMaxSesions;i++)
   {
      spSes=&pSessionArray[i];
      if(!spSes->cs.iBusy || spSes->cs.iCallStat>spSes->cs.EOk)continue;
      mb=spSes->mBase;
      CTMediaIDS *mids=spSes->pMediaIDS;
      if(!spSes->isSession() || !mb || !mids)continue;
//      if(z==mb->getZRTP() && z->pSes==spSes )return spSes;
      if(z==mids->pzrtp && z->pSes==spSes )return spSes;
      
   }
   return NULL;
}

CTZRTP *CPhSesions::findSessionZRTP(CSesBase *ses){
   CTSesMediaBase *mb;
   CSesBase *spSes;
   CTZRTP *z=NULL;
   
   for(int i=0;i<iMaxSesions;i++)
   {
      spSes=&pSessionArray[i];
      if(spSes!=ses || !spSes->cs.iBusy || spSes->cs.iCallStat>spSes->cs.EOk)continue;
      mb=spSes->mBase;
      CTMediaIDS *mids=spSes->pMediaIDS;
      if(!spSes->isSession() || !mb || !mids)break;
      z=mids->pzrtp;
      if(z && z->pSes==spSes){
         return z;
      }
      break;
   }
   return NULL;
}


CTAudioOutBase *CPhSesions::findSessionAO(CSesBase *ses){

   CTSesMediaBase *mb;
   
   CSesBase *spSes;
   
   if(!ses)return NULL;
   
   for(int i=0;i<iMaxSesions;i++)
   {
      spSes=&pSessionArray[i];
      if(!spSes->cs.iBusy || spSes!=ses)continue;
      mb=spSes->mBase;
      if(!spSes->isSession() || !mb)break;
      
      return findAOByMB(mb);
      
   }
   return NULL;
   
}

void CPhSesions::onDataSend(char *buf, int iLen, unsigned int uiPos, int iDataType, int iIsVoice)
{
    int i;
    if(bRun!=1)return;
    CSesBase *spSes;
#define dd_log //

    if(iLen>3 && buf){iRandomCounter*=((int)buf[1]+1000); iRandomCounter+=(9999+(int)buf[0])*29; iRandomCounter+=buf[2];}

    CTSesMediaBase *mb;
   
   for(i=0;i<iMaxSesions;i++)
   {
      spSes=&pSessionArray[i];
      if(!spSes->cs.iBusy ||
         (iDataType == CTSesMediaBase::eAudio && spSes->iIsInConference) || 
         spSes->cs.iCallStat!=CALL_STAT::EOk)continue ;
      
      mb=spSes->mBase;
      
      if(!mb || !spSes->isSession() || !spSes->isSesActive()  )continue;
      
      if(mb->isSesActive())mb->onSend(buf,iLen,iDataType,(void*)uiPos, iIsVoice);
   }
}

int makeSDP(CPhSesions &ph, CSesBase *spSes, CMakeSip &ms)
{

   STR_64 *vis=spSes->pIPVisble;
   int fUseExtPort=1;


   if(!spSes->mBase)return -1;//TODO ???

   int iTrue=(
      (spSes->mBase->uiIPConn && ph.extAddr.ip==spSes->mBase->uiIPConn) ||
      (spSes->mBase->uiIPConn && isNatIP(spSes->mBase->uiIPConn) && isNatIP(ph.ipBinded) &&
      (spSes->mBase->uiIPConn&0xffff)==((unsigned int)ph.ipBinded&0xffff)) ||
      (spSes->mBase->uiIPConn==0 && isNatIP(ph.ipBinded) && isNatIP(spSes->dstConAddr.ip))
      );
   /*
   if(sip && iTrue)
   {
      iTrue=iTrue && 
   }
   */
   if(iTrue && spSes->iDestIsNotInSameNet){
      iTrue=0;
   }

   if(iTrue)
   {
      //|| (spSes->mBase->uiIPConn==0 && natip))
      fUseExtPort=0;
      vis=&ph.str64BindedAddr;
   }
   else
   {
      if(spSes->cs.iSendS==METHOD_ACK)return 0;
      fUseExtPort=0;
//#ifdef USE_STUN
      if(ph.p_cfg.iUseStun &&  ph.p_cfg.iNet && (ph.p_cfg.iNet & (CTStun::FULL_CONE|CTStun::PORT_REST)))
      {
         fUseExtPort=1;
      }
//#endif
   }
   ms.makeSdpHdr(ph.str64BindedAddr.strVal,ph.str64BindedAddr.uiLen);


   if(ph.p_cfg.iUseStun)
      ms.addSdpConnectIP(vis->strVal,(int)vis->uiLen,&ph.p_cfg);
   else 
      ms.addSdpConnectIP(ph.str64BindedAddr.strVal,(int)ph.str64BindedAddr.uiLen,&ph.p_cfg);
   
   ms.makeSDPMedia(*spSes->mBase, ph.p_cfg.iUseStun &&  ph.p_cfg.iUseOnlyNatIp==0);// && fUseExtPort);
   
   return 0;
}
int CPhSesions::send200Sdp(CSesBase *spSes, SIP_MSG *sMsg)
{
   spSes->cs.iSendS=0;
   spSes->cs.iWaitS=METHOD_ACK;
   spSes->sSendTo.setRetransmit();


   CMakeSip ms(sockSip,spSes,sMsg);
   ms.makeResp(200,&p_cfg);
   makeSDP(*this,spSes,ms);
   ms.addContent();
   
   spSes->sSendTo.updateRetransmit(uiGT);

   if(sMsg && sMsg->hdrCSeq.uiMethodID!=METHOD_INVITE){
      spSes->cs.iWaitS=0;
      spSes->sSendTo.stopRetransmit();    
   }
   return sendSip(sockSip,spSes);
}

int CPhSesions::onSipMsgSes(CSesBase *spSes, SIP_MSG *sMsg)
{
   int iIsReq=sMsg->sipHdr.uiMethodID;
   int iMeth=sMsg->hdrCSeq.uiMethodID;

   if(sMsg->sipHdr.dstrStatusCode.uiVal>299 && sMsg->sipHdr.dstrStatusCode.uiVal!=491)//491 req pending
   {

      CMakeSip ms(sockSip,spSes,sMsg);

      if(sMsg->sipHdr.dstrStatusCode.uiVal<400)
      {
         ms.makeReq(iMeth,&p_cfg);
         if(iMeth==METHOD_INVITE)
         {
            makeSDP(*this,spSes,ms);
         }
         /*
         if(sMsg->sipHdr.dstrStatusCode.uiVal==305)//use px
         {
            if(sMsg->hldContact.x[0].sipUri.dstrHost.uiVal)
               spSes->dstConAddr.ip=sMsg->hldContact.x[0].sipUri.dstrHost.uiVal;
            spSes->dstConAddr.setPort(sMsg->hldContact.x[0].sipUri.dstrPort.uiVal);

         }
          */
         ms.addContent();
         sendSip(sockSip,spSes);
         return 0;

      }
      CTSesMediaBase *mb=spSes->mBase;
      if(sMsg->sipHdr.dstrStatusCode.uiVal!=481)
      {
         if(sMsg->sipHdr.dstrStatusCode.uiVal==400 && 
            //spSes->sSIPMsg.hldContact.x[0].sipUri.dstrSipAddr.uiLen==0 &&
            mb && 
            mb->getMediaType() & CTSesMediaBase::eVideo
           )
         {
            int iStart=mb->iIsActive;
            if(mb)
            {
              spSes->setMBase(NULL);
              cPhoneCallback->mediaFinder->release(mb);
            }
            spSes->setMBase(cPhoneCallback->mediaFinder->findMedia("audio",5));//TODO find bysdp
            if(spSes->mBase)
            {
               
               ms.makeReq(iMeth,&p_cfg);
               if(iStart)spSes->mBase->onStart();
               makeSDP(*this,spSes,ms);
               ms.addContent();
               sendSip(sockSip,spSes);

               return 0;
            }
            
         }
         ms.makeReq(METHOD_ACK,&p_cfg);
         ms.addContent();
         sendSip(sockSip,spSes);
      }
      if(iMeth==METHOD_INVITE)
      {
         onKillSes(spSes,-2,sMsg,iMeth);
      }


      return 0;
   }
   
  // spSes->
   
   if(spSes->cs.iSendS==METHOD_INVITE && sMsg->sipHdr.dstrStatusCode.uiVal==0 && iMeth==METHOD_INVITE &&  spSes->cs.iCallStat==CALL_STAT::EInit){
      
      keepAlive.sendEverySec(0);
      spSes->cs.iCallStat=CALL_STAT::EOk;
      DSTR *dstr=&sMsg->hdrFrom.sipUri.dstrSipAddr;
      if(p_cfg.iHideIP && sMsg->hdrFrom.sipUri.dstrUserName.uiLen){
         dstr=&sMsg->hdrFrom.sipUri.dstrUserName;
      }

      cPhoneCallback->onIncomCall(&spSes->dstConAddr,dstr->strVal,dstr->uiLen ,(int)spSes);
      spSes->mBase->onStart();
      cPhoneCallback->onStartCall((int)spSes);
      send200Sdp(spSes,sMsg);
      return 0;//???
   }
   
   if(spSes->cs.iCallStat==CALL_STAT::EEnding && iMeth==METHOD_INVITE)
   {
      if(iIsReq)
         CMakeSip ms(sockSip,603,sMsg);
      else
      {

         CMakeSip ms(sockSip, spSes);
         ms.makeReq(METHOD_BYE,&p_cfg);
         ms.addContent();
         sendSip(sockSip,spSes);
      }
      return 0;
   }


   if(sdpRec(sMsg,spSes)<0)return -1;
  



   switch(iMeth)
   {
   case METHOD_UPDATE:
         if(iIsReq){

            if(spSes->cs.iCallStat==CALL_STAT::EOk){
               send200Sdp(spSes,sMsg);
            }
            else {
               CMakeSip ms(sockSip,200,sMsg);
            }
         }
         else if(spSes->cs.iSendS==METHOD_UPDATE){
            spSes->sSendTo.stopRetransmit();
            spSes->cs.iSendS=0;
            spSes->cs.iWaitS=0;
         }
         break;
         
   case METHOD_INFO:
      if(iIsReq)
      {
         CMakeSip ms(sockSip,200,sMsg);
      }
      break;
   case METHOD_ACK:
      if (spSes->cs.iCallStat==CALL_STAT::ESendError && spSes->cs.iWaitS==METHOD_ACK)
      {
         onKillSes(spSes,0,NULL,0);
         return 0;
      }
      if (spSes->cs.iWaitS!=METHOD_ACK) 
      {
         DEBUG_T(0, "REc METHOD_ACK without waiting");
         return -1;
      }
      if (sMsg->sipHdr.uiMethodID!=METHOD_ACK)DEBUG_T(0, "sip hdr not ack");
      spSes->cs.iSendS=0;
      spSes->cs.iWaitS=0;
      spSes->sSendTo.stopRetransmit();
         
      spSes->dstConAddr=sMsg->addrSipMsgRec;
      break;

   case METHOD_BYE:
   case METHOD_CANCEL:
      if(iIsReq)
      {

         spSes->sSendTo.stopRetransmit();
         CMakeSip ms1(sockSip,200,sMsg);
         if(iMeth==METHOD_CANCEL){
            CMakeSip ms(sockSip,spSes,&spSes->sSIPMsg);
            ms.makeResp(487,&p_cfg);
            ms.addContent();
            sendSip(sockSip,spSes);
         }

         
      }

      onKillSes(spSes,0,NULL,0);

      return 0;
   case METHOD_INVITE:

      //TODO new get contact
      spSes->dstConAddr=sMsg->addrSipMsgRec;


      if ((spSes->sSIPMsg.hdrCSeq.uiMethodID==0 || spSes->sSIPMsg.hdrCSeq.dstrID.uiVal!=sMsg->hdrCSeq.dstrID.uiVal ||

         (spSes->sSIPMsg.hldContact.x[0].sipUri.dstrSipAddr.uiLen==0 && sMsg->hldContact.x[0].sipUri.dstrSipAddr.uiLen))
         ||(spSes->sSIPMsg.hdrCSeq.uiMethodID!=sMsg->hdrCSeq.uiMethodID && sMsg->hldContact.x[0].sipUri.dstrSipAddr.uiLen)
         ||(spSes->sSIPMsg.hldRecRoute.uiCount!=sMsg->hldRecRoute.uiCount || spSes->sSIPMsg.hldRecRoute.x[0].dstrFullRow.uiLen!=sMsg->hldRecRoute.x[0].dstrFullRow.uiLen)
         )
      {
         memset(&spSes->sSIPMsg,0,sizeof(SIP_MSG));
/*
         memcpy(spSes->sSIPMsg.rawDataBuffer, sMsg->rawDataBuffer, MSG_BUFFER_SIZE);
         spSes->sSIPMsg.uiOffset=sMsg->uiOffset;
  */       
         //parsing again, because we use pointers in spSes->sSIPMsg.rawDataBuffer
         cSip.parseSip(sMsg->rawDataBuffer, (int)(sMsg->uiOffset-MSG_BUFFER_TAIL_SIZE), &spSes->sSIPMsg);
 
      }
      //spSes->sSIPMsg.hdrCSeq.dstrID.uiVal=sMsg->hdrCSeq.dstrID.uiVal;
      
      switch(sMsg->sipHdr.dstrStatusCode.uiVal)
      {
         case 0:
            {
               if(spSes->cs.iCallStat==CALL_STAT::EOk)
               {
                  
                  send200Sdp(spSes,sMsg);
                  return 0;
               }
               
               CPY_DSTR(spSes->dstSipAddr,sMsg->hdrFrom.sipUri.dstrSipAddr,120);
               
               spSes->cs.iSendS=0;
               spSes->cs.iWaitS=0;
               
               spSes->sSendTo.stopRetransmit();
               if(p_cfg.iAutoAnswer)
               {
                  keepAlive.sendEverySec(0);
                  spSes->cs.iCallStat=CALL_STAT::EOk;
                  DSTR *dstr=&sMsg->hdrFrom.sipUri.dstrSipAddr;
                  if(p_cfg.iHideIP && sMsg->hdrFrom.sipUri.dstrUserName.uiLen){
                     dstr=&sMsg->hdrFrom.sipUri.dstrUserName;
                  }
                  
                  cPhoneCallback->onIncomCall(&spSes->dstConAddr,dstr->strVal,dstr->uiLen ,(int)spSes);
                  spSes->mBase->onStart();
                  cPhoneCallback->onStartCall((int)spSes);
                  
                  send200Sdp(spSes,sMsg);
                  return 0;
               }
               keepAlive.sendEverySec(1);
               
               CMakeSip ms(sockSip,spSes,sMsg);//TODO use new
               ms.makeResp(180,&p_cfg);
               ms.addContent();
               sendSip(sockSip,spSes);
               
            }
            if(spSes->cs.iCallSubStat!=CALL_STAT::EWaitUserAnswer)
            {
               spSes->cs.iCallSubStat=CALL_STAT::EWaitUserAnswer;

               DSTR *dstr=&sMsg->hdrFrom.sipUri.dstrSipAddr;
               if(p_cfg.iHideIP && sMsg->hdrFrom.sipUri.dstrUserName.uiLen){
                  dstr=&sMsg->hdrFrom.sipUri.dstrUserName;
               }

               cPhoneCallback->onIncomCall(&spSes->dstConAddr,dstr->strVal,dstr->uiLen ,(int)spSes);
               spSes->uiSend480AfterTicks=50;//sends sip 480 after ~1min

            }

            break;
         case 200:
            {
               
               keepAlive.sendEverySec(0);
               
               spSes->sSendTo.stopRetransmit();
               
               spSes->cs.iSendS=METHOD_ACK;
               spSes->cs.iWaitS=0;
               
               CMakeSip ms(sockSip,spSes,sMsg);
               ms.makeReq(METHOD_ACK,&p_cfg);
               
               //    if(!p_cfg.iUseOnlyNatIp)
               //     makeSDP(*this,spSes,ms);//TODO cfg if (UA not  tivi){repl with reinvite}
               ms.addContent();
               sendSip(sockSip,spSes);
               
               if(spSes->cs.iCallStat!=CALL_STAT::EOk)
               {
                  spSes->cs.iCallStat=CALL_STAT::EOk;
                  cPhoneCallback->onStartCall((int)spSes);
               }
               if(spSes->mBase)spSes->mBase->onRecMsg(200);
            }
            break;
         case 491:
            //TODO inc retransmit timer
            {
               CMakeSip ms(sockSip,spSes,sMsg);
               ms.makeReq(METHOD_ACK,&p_cfg);
               ms.addContent();
               sendSip(sockSip,spSes);
            }
         case 100:
         case 180:
         case 183:
            if(sMsg->sipHdr.dstrStatusCode.uiVal==183 ){
               cPhoneCallback->onRecMsg(183,(int)spSes,NULL,0);
            }
            else if(sMsg->sipHdr.dstrStatusCode.uiVal==180 )//TODO && no sdp
            {
               if(spSes->mBase)spSes->mBase->onRecMsg(180);
               cPhoneCallback->onRecMsg(180,(int)spSes,NULL,0);
            }
            else{
               cPhoneCallback->onRecMsg((int)sMsg->sipHdr.dstrStatusCode.uiVal,(int)spSes,sMsg->sipHdr.dstrReasonPhrase.strVal,(int)sMsg->sipHdr.dstrReasonPhrase.uiLen);
            }
         default:
            if((sMsg->sipHdr.dstrStatusCode.uiVal>=100 && sMsg->sipHdr.dstrStatusCode.uiVal<200) || (sMsg->sipHdr.dstrStatusCode.uiVal==491))
            {
               spSes->cs.iSendS=0;
               spSes->cs.iWaitS=200;
               spSes->sSendTo.stopRetransmit();
            }
            else{
               cPhoneCallback->onRecMsg((int)sMsg->sipHdr.dstrStatusCode.uiVal,(int)spSes,sMsg->sipHdr.dstrReasonPhrase.strVal,(int)sMsg->sipHdr.dstrReasonPhrase.uiLen);
            }
      }
      break;
   default:
      return -1;
   }
 //  if(sdpRec(sMsg,spSes)<0)return -1;
   //if(sdp)


   return 0;
}
int CPhSesions::onSipMsg(CSesBase *spSes, SIP_MSG *sMsg)
{
   //int iIsReq=sMsg->sipHdr.uiMethodID;
   int iMeth=sMsg->hdrCSeq.uiMethodID;
   int iSipCode=sMsg->sipHdr.dstrStatusCode.uiVal;

   //cPhoneCallback->message(sMsg->
   if (iMeth & (METHOD_MESSAGE|METHOD_OPTIONS|METHOD_SUBSCRIBE|METHOD_PUBLISH))
   {
      if(spSes->ptrResp){
         if(iSipCode!=100)
            *spSes->ptrResp=iSipCode;
         //printf("*spSes->ptrResp=%d\n",*spSes->ptrResp);
      }

      SEND_TO *st=&spSes->sSendTo;
      if(iSipCode>299 && iMeth==METHOD_MESSAGE && iSipCode<400 
         && sMsg->hldContact.x[0].sipUri.dstrSipAddr.uiLen 
         &&  st->pContentType && st->pContent && st->iContentLen)
      {
         int i;
         st=new SEND_TO;
         //varbuut sesijaam pashaam sadaliities
         //taisiit memcpy
         //TODO pazinjot sho galvenajam
         //var lietot arii memmove
         memcpy(st,&spSes->sSendTo,sizeof(SEND_TO));
         st->pContent-=((int)&spSes->sSendTo-(int)st);

         for(i=0;i<(int)sMsg->hldContact.uiCount;i++)
         {
            CMakeSip ms(sockSip,spSes,sMsg);
            ms.iContactId=i;
            ms.makeReq(iMeth,&p_cfg);
            ms.addContent(st->pContentType, st->pContent, st->iContentLen);
            //
            spSes->sSendTo.iRetransmitPeriod=st->iRetransmitPeriod;
            spSes->sSendTo.iRetransmitions=st->iRetransmitions;
            sendSip(sockSip,spSes);
         }
         delete st;
         return 0;
      }
      spSes->cs.iWaitS=0;
      if (iSipCode==100)
      {
         spSes->sSendTo.stopRetransmit();
         return 0;
      }
      if(iSipCode>299 && iMeth==METHOD_MESSAGE)
      {

         onKillSes(spSes,-2,sMsg,METHOD_MESSAGE);

      }
      if(iSipCode>299 && iMeth==METHOD_SUBSCRIBE)
      {
         onKillSes(spSes,-2,sMsg,METHOD_SUBSCRIBE);
      }
      //if 200 set new contact info
      if(iMeth==METHOD_OPTIONS)
         iOptionsSent=2;
      freeSes(spSes);
      return 0;
   }
   if (iMeth==METHOD_REGISTER)
   {
      if (iSipCode==100)
      {
         spSes->cs.iWaitS=0;
         return 0;
      }
      if (spSes->cs.iSendS!=METHOD_REGISTER)// || spSes->cs.iWaitS!=200)
         return -1;
      if (iSipCode==200)
      {
         unsigned int i;
         
         if(!p_cfg.reg.bUnReg)
         {
            
            int uiExpTime=p_cfg.uiExpires;
            int ok=0;
            int b=0;         
            spSes->cs.iWaitS=0;
            if (sMsg->dstrExpires.uiVal>0)
               uiExpTime=sMsg->dstrExpires.uiVal;
            else 
            {
               ok=1;
               
               for(i=0;i<sMsg->hldContact.uiCount;i++)
               {
                  if (sMsg->hldContact.x[i].dstrExpires.uiVal>0)
                     b=sMsg->hldContact.x[i].dstrExpires.uiVal;
                  
               //   printf("[hosts (%.*s)(%.*s) %d]",spSes->pIPVisble->uiLen,spSes->pIPVisble->strVal, sMsg->hldContact.x[i].sipUri.dstrHost.uiLen, sMsg->hldContact.x[i].sipUri.dstrHost.strVal,spSes->uiUserVisibleSipPort==sMsg->hldContact.x[i].sipUri.dstrPort.uiVal);
                  
#define IS_EQUAL_DSTR(a,b) ((a).uiLen==(b).uiLen && memcmp((a).strVal,(b).strVal,(b).uiLen)==0)
                  if(IS_EQUAL_DSTR(sMsg->hldContact.x[i].sipUri.dstrHost,
                                   *spSes->pIPVisble) && 
                     spSes->uiUserVisibleSipPort==sMsg->hldContact.x[i].sipUri.dstrPort.uiVal)
                  {
                     if (sMsg->hldContact.x[i].dstrExpires.uiVal>0)
                     {
                        ok=2;
                        uiExpTime=b;//sMsg->hldContact.x[i].dstrExpires.uiVal;
                        DEBUG_T(0,"reg-cont-found =ok")
                     }
                     break;
                  }
               }
            }
            if(ok==1 && b)
               uiExpTime=b;
            
            p_cfg.iReRegisterNow=0;
            
            uiNextRegTry = uiGT+((uiExpTime*10*T_GT_SECOND)>>4);
            p_cfg.reg.uiRegUntil=uiGT+(uiExpTime*15*T_GT_SECOND>>4);
            printf("[exp %d,%d,%lld,%d %s]",uiExpTime,b,(p_cfg.reg.uiRegUntil-uiGT)/T_GT_SECOND,p_cfg.uiExpires, p_cfg.str32GWaddr.strVal);
            p_cfg.iCanRegister=1;
            iRegTryParam=0;
            
            cPhoneCallback->registrationInfo(&strings.lRegSucc,cPhoneCallback->eOnline);
            
            //TODO if(prev_contact_addr!=curr_contact_addr)reInvite(); here
            int _TODO_if_contact_addr_changed_reinvite_here;
         }
         else
         {
            iRegTryParam=0;
            //TODO rem addr
            CTEditBuf<16>  b;
            b.setText("---Offline---");
            cPhoneCallback->registrationInfo(&b, cPhoneCallback->eOffline);
            
            if(p_cfg.iUserRegister){
               uiNextRegTry=uiGT+T_GT_SECOND*10;//10sec
            }
            
            p_cfg.reg.uiRegUntil=0;
            if(p_cfg.changeUserParams.iSeq==2)
               p_cfg.changeUserParams.iSeq=3;
         }
         p_cfg.reg.bReReg=0;
         p_cfg.reg.bRegistring=0;
         p_cfg.reg.bUnReg=0;
         if(!p_cfg.iUserRegister)uiNextRegTry=0;
         
         
         spSes->cs.iWaitS=0;
         freeSes(spSes);
         return 0;
      }
      if (iSipCode>200)
      {
         spSes->cs.iWaitS=0;
         CMakeSip ms(sockSip,spSes,sMsg);
         ms.makeReq(METHOD_ACK,&p_cfg);
         ms.addContent();

         sendSip(sockSip,spSes);
         onKillSes(spSes,-2,sMsg,METHOD_REGISTER);

         return 0;
      }
      else
      {
         spSes->cs.iWaitS=0;
      }
   }

   return 0;
}
//int parseSDP(SDP *pRecSDP, char *pStart, int iLen);
int CPhSesions::sdpRec(SIP_MSG *sMsg,CSesBase *spSes)
{
   if (sMsg->dstrContLen.uiVal==0 || spSes->uiClearMemAt ||
       sMsg->hdrContType.uiTypeID!=CONTENT_TYPE_APPLICATION || 
      !(CMP(sMsg->hdrContType.dstrSubType,"SDP",3)))
      return 0;
   
   

   if(spSes->mBase)
   {
      int iPrevIsAudioOnly=spSes->mBase->getMediaType()==spSes->mBase->eAudio;
      //TODO parse SDP once, spSes->mBase->onSdp(sdp);
      int not_ok=spSes->mBase->onSdp(sMsg->rawDataBuffer+sMsg->uiBytesParsed+1,(int) sMsg->dstrContLen.uiVal);
      //TODO error codes
      CTSesMediaBase *mb=NULL;
      if(not_ok)//mainaas sdp media tips 
      {
         int iStart=spSes->mBase->iIsActive;
         
         mb=spSes->mBase;
         if(mb)
         {
           //spSes->mBase=NULL;
            spSes->setMBase(NULL);
         }
         CTSesMediaBase *nb=cPhoneCallback->mediaFinder->findMedia(iPrevIsAudioOnly?"video":"audio",5);
//TODO find by sdp
         if(!nb && iPrevIsAudioOnly)nb=cPhoneCallback->mediaFinder->findMedia("audio",5);
         spSes->setMBase(nb);//TODO find bysdp
         if(spSes->mBase)
         {

            not_ok=spSes->mBase->onSdp(sMsg->rawDataBuffer+sMsg->uiBytesParsed+1,(int) sMsg->dstrContLen.uiVal);
            if(not_ok)
            {
               CTSesMediaBase *mb=spSes->mBase;
               if(mb)
               {
                 //spSes->mBase=NULL;
                  spSes->setMBase(NULL);
                 cPhoneCallback->mediaFinder->release(mb);
               }
            }
            else if(iStart)spSes->mBase->onStart();
         }

      }
      if(!not_ok && spSes->mBase){
         cPhoneCallback->onSdpMedia((int)spSes,spSes->mBase->getMediaName());
      }
      if(mb)cPhoneCallback->mediaFinder->release(mb);


      if(not_ok)
      {
         if(spSes->sSIPMsg.uiOffset)
         {
            spSes->cs.iSendS=415;
            spSes->cs.iWaitS=METHOD_ACK;

            spSes->cs.iCallStat=CALL_STAT::ESendError;

            CMakeSip ms(sockSip,spSes,sMsg);
            ms.makeResp(spSes->cs.iSendS,&p_cfg);
            ms.addContent();
            sendSip(sockSip,spSes);
         }
         else
         {
            CMakeSip ms(sockSip);
            ms.cPhoneCallback=cPhoneCallback;
            ms.sendResp(sockSip,415,sMsg);
            onKillSes(spSes,0,NULL,0);
         }
      }
      return not_ok;
   }
   return 0;
}

