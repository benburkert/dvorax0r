#include "ListFireRelativePointer.hpp"
#include "RemapUtil.hpp"

namespace org_pqrs_KeyRemap4MacBook {
  void
  ListFireRelativePointer::add(PointingButton::PointingButton button)
  {
    if (size >= MAXNUM) return;
    list[size].set(button);
    ++size;
  }

  void
  ListFireRelativePointer::fire(RelativePointerEventCallback callback, OSObject *target, OSObject *sender, AbsoluteTime ts)
  {
    if (callback == NULL) return;

    for (int i = 0; i < size; ++i) {
      Item &item = list[i];

      Params_RelativePointerEventCallback callbackparams = {
        target,
        item.getButton(),
        0, 0,
        ts,sender, 0,
      };
      RemapUtil::execCallBack_RelativePointerEventCallBack(callback, callbackparams);
    }
  }
}
