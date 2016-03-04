#include "watchdog/public/IWdClntWrapper.h"
#include "common/IChangeNodeState.h"

class CWdClntWrapper : public IWdClntWrapper {
public:
   virtual ~CWdClntWrapper() {;}
   virtual void wait();
   virtual void finish() {;}
   virtual IWdClient * getWdClient() { return nullptr; }
private:
   friend IWdClntWrapper;

   CWdClntWrapper(std::string wdName, std::string logName, 
      CApiEndpointBuilder& ebBuildef, zmq::context_t * pCtx){;}
};

IWdClntWrapper * IWdClntWrapper::createWdClntWrapper(std::string wdName, std::string logName,  
                                                     CApiEndpointBuilder& ebBuildef, zmq::context_t * pCtx)
{
   return new CWdClntWrapper(wdName, logName, ebBuildef, pCtx);
}

void CWdClntWrapper::wait()
{
   for(;;) {
      IChangeNodeState::result_t nodeState =  IChangeNodeState::getChangeNodeStateObj().waitSignal();
      if (nodeState == IChangeNodeState::STOPPED)
         break;
   }
}
