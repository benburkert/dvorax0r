#ifndef FLAGSTATUS_HPP
#define FLAGSTATUS_HPP

#include "KeyCode.hpp"

namespace org_pqrs_KeyRemap4MacBook {
  class FlagStatus {
  public:
    class Item {
      friend class FlagStatus;

    public:
      Item(void) : flag_(ModifierFlag::NONE) {}

    private:
      void initialize(const ModifierFlag& f);
      void set(void);
      void set(const KeyCode& key, const Flags& flags);

      void reset(void);
      Flags makeFlag(void) const {
        if (count_ + temporary_count_ + lock_count_ + original_lock_count_ > 0) {
          return flag_;
        } else {
          return 0;
        }
      }

      void increase(void);
      void decrease(void);
      void temporary_increase(void) { ++temporary_count_; }
      void temporary_decrease(void) { --temporary_count_; }
      void lock_increase(void) { lock_count_ = 1; }
      void lock_decrease(void) { lock_count_ = 0; }

      ModifierFlag flag_;
      int count_;
      int temporary_count_;

      int lock_count_; // store remapped lock status. (CapsLock, FN lock, ...)
      int original_lock_count_; // store original CapsLock status.
    };
    enum { MAXNUM = 16 };

    static bool initialize(void);
    static void set(void);
    static void set(const KeyCode& key, const Flags& flags);
    static Flags makeFlags(void);
    // get registered ModifierFlag by index.
    static const ModifierFlag& getFlag(int index);
    static void reset(void);

    static void increase(const Flags& flags);
    static void decrease(const Flags& flags);
    static void temporary_increase(const Flags& flags);
    static void temporary_decrease(const Flags& flags);
    static void lock_increase(const Flags& flags);
    static void lock_decrease(const Flags& flags);

  private:
    static Item item_[MAXNUM];
  };
}

#endif
