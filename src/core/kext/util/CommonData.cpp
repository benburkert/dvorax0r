#include "CommonData.hpp"
#include "IOLockWrapper.hpp"
#include "Config.hpp"
#include "Client.hpp"

namespace org_pqrs_KeyRemap4MacBook {
  AbsoluteTime CommonData::current_ts_;
  KeyboardType CommonData::current_keyboardType_;
  DeviceVendor CommonData::current_deviceVendor_;
  DeviceProduct CommonData::current_deviceProduct_;
  KeyRemap4MacBook_bridge::GetWorkspaceData::Reply CommonData::current_workspacedata_;

  int CommonData::alloccount_;
  IOLock* CommonData::alloccount_lock_;

  IOLock* CommonData::event_lock_;

  bool
  CommonData::initialize(void)
  {
    alloccount_lock_ = IOLockWrapper::alloc();
    event_lock_ = IOLockWrapper::alloc();
    return true;
  }

  void
  CommonData::terminate(void)
  {
    IOLockWrapper::free(alloccount_lock_);
    IOLockWrapper::free(event_lock_);
  }

  void
  CommonData::setcurrent_workspacedata(void)
  {
    // ------------------------------------------------------------
    // When we press the functional key (ex. F2) with the keyboard of the third vendor,
    // KeyRemap4MacBook_client::sendmsg returns EIO.
    //
    // We use the previous value when the error occurred.
    static KeyRemap4MacBook_bridge::GetWorkspaceData::Reply last = {
      0,
      0,
      0,
    };

    int error = KeyRemap4MacBook_client::sendmsg(KeyRemap4MacBook_bridge::REQUEST_GET_WORKSPACE_DATA, NULL, 0, &current_workspacedata_, sizeof(current_workspacedata_));
    IOLOG_DEVEL("GetWorkspaceData: type:%d, inputmode:%d, inputmodedetail:%d, error:%d\n",
                current_workspacedata_.type,
                current_workspacedata_.inputmode,
                current_workspacedata_.inputmodedetail,
                error);
    if (error == 0) {
      last = current_workspacedata_;
    } else {
      // use last info.
      current_workspacedata_ = last;
    }
  }

  void
  CommonData::increase_alloccount(void)
  {
    IOLockWrapper::ScopedLock lk(alloccount_lock_);
    if (! lk) return;

    ++alloccount_;
    if (alloccount_ > 1024) {
      IOLOG_WARN("alloccount_ > 1024\n");
    }
    //IOLOG_DEVEL("CommonData::increase_alloccount alloccount_:%d\n", alloccount_);
  }

  void
  CommonData::decrease_alloccount(void)
  {
    IOLockWrapper::ScopedLock lk(alloccount_lock_);
    if (! lk) return;

    --alloccount_;
    //IOLOG_DEVEL("CommonData::decrease_alloccount alloccount_:%d\n", alloccount_);
  }
}
