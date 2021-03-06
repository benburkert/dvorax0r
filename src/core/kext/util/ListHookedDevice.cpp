#include <IOKit/hid/IOHIDKeys.h>
#include "Config.hpp"
#include "IOLockWrapper.hpp"
#include "ListHookedDevice.hpp"
#include "NumHeldDownKeys.hpp"

namespace org_pqrs_KeyRemap4MacBook {
  // ======================================================================
  ListHookedDevice::Item::Item(IOHIDevice* d) : device_(d), vendor_(0), product_(0), deviceType_(DeviceType::UNKNOWN) {
    setVendorProduct();
    setDeviceType();
  }

  bool
  ListHookedDevice::Item::isEqualVendorProduct(DeviceVendor vendor, DeviceProduct product) const
  {
    return vendor_ == vendor && product_ == product;
  }

  bool
  ListHookedDevice::Item::isEqualVendor(DeviceVendor vendor) const
  {
    return vendor_ == vendor;
  }

  void
  ListHookedDevice::Item::setVendorProduct(void)
  {
    if (! device_) return;

    IORegistryEntry* dev = device_;

    while (dev) {
      const OSNumber* vid = NULL;
      vid = OSDynamicCast(OSNumber, dev->getProperty(kIOHIDVendorIDKey));

      const OSNumber* pid = NULL;
      pid = OSDynamicCast(OSNumber, dev->getProperty(kIOHIDProductIDKey));

      if (vid && pid) {
        vendor_ = vid->unsigned32BitValue();
        product_ = pid->unsigned32BitValue();

        goto finish;
      }

      // check parent property.
      dev = dev->getParentEntry(IORegistryEntry::getPlane(kIOServicePlane));
    }

  finish:
    IOLOG_DEBUG("HookedDevice::setVendorProduct device_:%p, vendor_:0x%04x, product_:0x%04x\n", device_, vendor_.get(), product_.get());
  }

  void
  ListHookedDevice::Item::setDeviceType(void)
  {
    if (! device_) return;

    const char* name = NULL;

    name = device_->getName();
    if (! name) goto finish;

    // Apple device
    if (strcmp(name, "IOHIDKeyboard")         == 0 ||
        strcmp(name, "AppleADBKeyboard")      == 0 ||
        strcmp(name, "IOHIDConsumer")         == 0 ||
        strcmp(name, "IOHIDPointing")         == 0 ||
        strcmp(name, "AppleUSBGrIIITrackpad") == 0 ||
        strcmp(name, "AppleADBMouseType4")    == 0) {

      deviceType_ = DeviceType::APPLE_EXTERNAL;

      // ------------------------------------------------------------
      // Judge internal device or external device.
      //
      // We judge it from a product name whether it is internal device.
      // At keyboard, we cannot use the KeyboardType,
      // because some external keyboard has the same KeyboardType as Apple internal keyboard.
      const OSString* productname = NULL;
      productname = OSDynamicCast(OSString, device_->getProperty(kIOHIDProductKey));
      if (productname) {
        const char* pname = productname->getCStringNoCopy();
        if (pname) {
          const char* internalname = "Apple Internal ";
          if (strncmp(internalname, pname, strlen(internalname)) == 0) {
            deviceType_ = DeviceType::APPLE_INTERNAL;
          }
        }
      }

      goto finish;
    }

    // USB Overdrive
    if (strcmp(name, "OverdriveHIDKeyboard") == 0 ||
        strcmp(name, "OverdriveHIDPointer")  == 0) {
      deviceType_ = DeviceType::USB_OVERDRIVE;
      goto finish;
    }

  finish:
    IOLOG_DEBUG("HookedDevice::setDeviceType device_:%p, name:%s, deviceType_:%d\n",
                device_,
                name ? name : "null",
                deviceType_);
  }

  bool
  ListHookedDevice::Item::isConsumer(const char* name)
  {
    if (! name) return false;

    if (strcmp(name, "IOHIDConsumer") == 0) return true;
    if (strcmp(name, "com_Logitech_HISpecialKeys") == 0) return true;

    return false;
  }

  // ======================================================================
  namespace {
    void
    reset(void)
    {
      NumHeldDownKeys::reset();
    }
  }

  bool
  ListHookedDevice::initialize(void)
  {
    list_lock_ = IOLockWrapper::alloc();
    if (! list_lock_) return false;

    list_ = new List();
    if (! list_) return false;

    return true;
  }

  void
  ListHookedDevice::terminate(void)
  {
    {
      IOLockWrapper::ScopedLock lk(list_lock_);

      if (list_) {
        delete list_;
      }

      reset();
    }
    IOLockWrapper::free(list_lock_);
  }

  void
  ListHookedDevice::push_back(ListHookedDevice::Item* newp)
  {
    IOLockWrapper::ScopedLock lk(list_lock_);
    if (! lk) return;

    if (! list_) return;
    if (! newp) return;

    last_ = newp->device_;
    list_->push_back(newp);

    // Call reset whenever the device status is changed.
    reset();

    IOLOG_DEVEL("ListHookedDevice::push_back list_->size = %d\n", static_cast<int>(list_->size()));
  }

  void
  ListHookedDevice::erase(IOHIDevice* p)
  {
    IOLockWrapper::ScopedLock lk(list_lock_);
    if (! lk) return;

    if (! list_) return;

    ListHookedDevice::Item* item = get_nolock(p);
    if (! item) return;

    list_->erase(item);

    // Call reset whenever the device status is changed.
    reset();

    IOLOG_DEVEL("ListHookedDevice::erase list_->size = %d\n", static_cast<int>(list_->size()));
  }

  ListHookedDevice::Item*
  ListHookedDevice::get_nolock(const IOHIDevice* device)
  {
    if (! list_) return NULL;

    last_ = device;

    for (Item* p = static_cast<Item*>(list_->front()); p; p = static_cast<Item*>(p->getnext())) {
      if (p->device_ == device) return p;
    }

    return NULL;
  }

  ListHookedDevice::Item*
  ListHookedDevice::get_replaced_nolock(void)
  {
    if (! list_) return NULL;

    // ----------------------------------------------------------------------
    // Search a replaced device first.
    ListHookedDevice::Item* p = get_nolock(last_);
    if (p && p->isReplaced()) return p;

    for (p = static_cast<Item*>(list_->front()); p; p = static_cast<Item*>(p->getnext())) {
      if (p->isReplaced()) return p;
    }

    return NULL;
  }

  void
  ListHookedDevice::refresh_callback(void)
  {
    IOLockWrapper::ScopedLock lk(list_lock_);
    if (! lk) return;

    if (! list_) return;

    for (Item* p = static_cast<Item*>(list_->front()); p; p = static_cast<Item*>(p->getnext())) {
      if (p->refresh_callback()) {
        // Call reset whenever the device status is changed.
        reset();
      }
    }
  }
}
