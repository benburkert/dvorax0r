#ifndef CONSUMERTOKEY_HPP
#define CONSUMERTOKEY_HPP

#include "RemapFuncBase.hpp"
#include "KeyToKey.hpp"
#include "ConsumerToConsumer.hpp"

namespace org_pqrs_KeyRemap4MacBook {
  namespace RemapFunc {
    class ConsumerToKey {
    public:
      ConsumerToKey(void) : index_(0) {}
      bool remap(RemapConsumerParams& remapParams);
      void initialize(void);
      void terminate(void);

      // [0] => fromKey
      // [1] => toKeys[0]
      // [2] => toKeys[1]
      // [3] => ...
      void add(ConsumerKeyCode newval);
      void add(KeyCode newval);
      void add(Flags newval);

    private:
      size_t index_;
      FromKeyChecker fromkeychecker_;
      PairConsumerKeyFlags fromKey_;

      KeyToKey keytokey_;
      ConsumerToConsumer consumertoconsumer_;
    };
  }
}

#endif
