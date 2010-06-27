#include "CommonData.hpp"
#include "Config.hpp"
#include "EventOutput.hpp"
#include "FlagStatus.hpp"
#include "KeyCode.hpp"
#include "RemapClass.hpp"
#include "RemapUtil.hpp"
#include "util/KeyboardRepeat.hpp"
#include "util/ListHookedKeyboard.hpp"
#include "VirtualKey.hpp"

namespace org_pqrs_KeyRemap4MacBook {
  void
  RemapUtil::KeyToKey::initialize(void)
  {
    toKeys_ = new Vector_PairKeyFlags();
  }

  void
  RemapUtil::KeyToKey::terminate(void)
  {
    if (toKeys_) {
      delete toKeys_;
      toKeys_ = NULL;
    }
  }

  void
  RemapUtil::KeyToKey::add(KeyCode newval)
  {
    switch (index_) {
      case 0:
        fromKey_.key = newval;
        break;
      default:
        if (toKeys_) {
          toKeys_->push_back(PairKeyFlags(newval));
        }
        break;
    }
    ++index_;
  }

  void
  RemapUtil::KeyToKey::add(Flags newval)
  {
    switch (index_) {
      case 0:
        IOLOG_ERROR("Invalid KeyToKey::add\n");
        break;
      case 1:
        fromKey_.flags = newval;
        break;
      default:
        if (toKeys_) {
          (toKeys_->back()).flags = newval;
        }
        break;
    }
  }

  bool
  RemapUtil::KeyToKey::remap(RemapParams& remapParams)
  {
    if (! toKeys_) return false;

    if (remapParams.isremapped) return false;
    if (! fromkeychecker_.isFromKey(remapParams, fromKey_.key, fromKey_.flags)) return false;
    remapParams.isremapped = true;

    // ------------------------------------------------------------
    // handle EventType & Modifiers
    bool isKeyDown = remapParams.isKeyDownOrModifierDown();
    if (isKeyDown) {
      FlagStatus::decrease(fromKey_.key.getModifierFlag());
    } else {
      FlagStatus::increase(fromKey_.key.getModifierFlag());
    }
    KeyboardRepeat::cancel();

    switch (toKeys_->size()) {
      case 0:
        break;

      case 1:
      {
        EventType newEventType = isKeyDown ? EventType::DOWN : EventType::UP;
        ModifierFlag toModifierFlag = (*toKeys_)[0].key.getModifierFlag();

        if (toModifierFlag == ModifierFlag::NONE) {
          // toKey
          FlagStatus::temporary_decrease(fromKey_.flags);
          FlagStatus::temporary_increase((*toKeys_)[0].flags);

        } else {
          // toModifier
          newEventType = EventType::MODIFY;

          if (isKeyDown) {
            FlagStatus::increase((*toKeys_)[0].flags | toModifierFlag);
            FlagStatus::decrease(fromKey_.flags);
          } else {
            FlagStatus::decrease((*toKeys_)[0].flags | toModifierFlag);
            FlagStatus::increase(fromKey_.flags);
          }
        }

        // ----------------------------------------
        Params_KeyboardEventCallBack::auto_ptr ptr(Params_KeyboardEventCallBack::alloc(newEventType,
                                                                                       FlagStatus::makeFlags(),
                                                                                       (*toKeys_)[0].key,
                                                                                       remapParams.params.keyboardType,
                                                                                       remapParams.params.repeat));
        if (! ptr) return false;
        Params_KeyboardEventCallBack& params = *ptr;

        KeyboardRepeat::set(params);
        EventOutput::FireKey::fire(params);

        break;
      }

      default:
        if (isKeyDown) {
          FlagStatus::temporary_decrease(fromKey_.flags);

          for (size_t i = 0; i < toKeys_->size(); ++i) {
            FlagStatus::temporary_increase((*toKeys_)[i].flags);

            Flags f = FlagStatus::makeFlags();
            KeyboardType keyboardType = remapParams.params.keyboardType;

            EventOutput::FireKey::fire_downup(f, (*toKeys_)[i].key, keyboardType);
            KeyboardRepeat::primitive_add(EventType::DOWN, f, (*toKeys_)[i].key, keyboardType);
            KeyboardRepeat::primitive_add(EventType::UP,   f, (*toKeys_)[i].key, keyboardType);

            FlagStatus::temporary_decrease((*toKeys_)[i].flags);
          }

          KeyboardRepeat::primitive_start();
        }
        break;
    }

    return true;
  }

  bool
  RemapUtil::KeyToKey::remap(RemapParams& remapParams,
                             KeyCode fromKeyCode, Flags fromFlags,
                             KeyCode toKeyCode,   Flags toFlags,
                             bool isSetKeyRepeat)
  {
    if (remapParams.isremapped) return false;
    if (! fromkeychecker_.isFromKey(remapParams, fromKeyCode, fromFlags)) return false;
    remapParams.isremapped = true;

    // ------------------------------------------------------------
    // handle EventType & Modifiers
    bool isKeyDown = remapParams.isKeyDownOrModifierDown();
    if (isKeyDown) {
      FlagStatus::decrease(fromKeyCode.getModifierFlag());
    } else {
      FlagStatus::increase(fromKeyCode.getModifierFlag());
    }

    // ----------------------------------------
    EventType newEventType = isKeyDown ? EventType::DOWN : EventType::UP;
    ModifierFlag toModifierFlag = toKeyCode.getModifierFlag();

    if (toModifierFlag == ModifierFlag::NONE) {
      // toKey
      FlagStatus::temporary_decrease(fromFlags);
      FlagStatus::temporary_increase(toFlags);

    } else {
      // toModifier
      newEventType = EventType::MODIFY;

      if (isKeyDown) {
        FlagStatus::increase(toFlags | toModifierFlag);
        FlagStatus::decrease(fromFlags);
      } else {
        FlagStatus::decrease(toFlags | toModifierFlag);
        FlagStatus::increase(fromFlags);
      }
    }

    // ----------------------------------------
    Params_KeyboardEventCallBack::auto_ptr ptr(Params_KeyboardEventCallBack::alloc(newEventType,
                                                                                   FlagStatus::makeFlags(),
                                                                                   toKeyCode,
                                                                                   remapParams.params.keyboardType,
                                                                                   remapParams.params.repeat));
    if (! ptr) return false;
    Params_KeyboardEventCallBack& params = *ptr;

    if (isSetKeyRepeat) {
      KeyboardRepeat::set(params);
    }
    EventOutput::FireKey::fire(params);

    return true;
  }

  namespace {
    void
    firekeycombination(const RemapParams& remapParams, KeyCode key, Flags flags)
    {
      FlagStatus::temporary_increase(flags);

      Flags f = FlagStatus::makeFlags();
      KeyboardType keyboardType = remapParams.params.keyboardType;

      EventOutput::FireKey::fire_downup(f, key, keyboardType);
      KeyboardRepeat::primitive_add(EventType::DOWN, f, key, keyboardType);
      KeyboardRepeat::primitive_add(EventType::UP, f, key, keyboardType);

      FlagStatus::temporary_decrease(flags);
    }
  }

  bool
  RemapUtil::KeyToKey::remap(RemapParams& remapParams,
                             KeyCode fromKeyCode, Flags fromFlags,
                             KeyCode toKeyCode1,  Flags toFlags1,
                             KeyCode toKeyCode2,  Flags toFlags2,
                             KeyCode toKeyCode3,  Flags toFlags3,
                             KeyCode toKeyCode4,  Flags toFlags4,
                             KeyCode toKeyCode5,  Flags toFlags5)
  {
    bool isKeyDown = remapParams.isKeyDownOrModifierDown();

    // We convert it into VK_NONE about the real hardware key.
    // This is necessary to output toKeyCode1 - toKeyCode5 at the KeyDown event.
    bool result = remap(remapParams, fromKeyCode, fromFlags, KeyCode::VK_NONE, ModifierFlag::NONE, false);
    if (! result) return false;

    KeyboardRepeat::cancel();

    if (isKeyDown) {
      firekeycombination(remapParams, toKeyCode1, toFlags1);
      firekeycombination(remapParams, toKeyCode2, toFlags2);
      firekeycombination(remapParams, toKeyCode3, toFlags3);
      firekeycombination(remapParams, toKeyCode4, toFlags4);
      firekeycombination(remapParams, toKeyCode5, toFlags5);

      KeyboardRepeat::primitive_start();
    }

    return true;
  }

  void
  RemapUtil::ConsumerToKey::initialize(void)
  {
    keytokey_.initialize();
    consumertoconsumer_.initialize();
    toKeys_ = new Vector_PairKeyFlags();
  }

  void
  RemapUtil::ConsumerToKey::terminate(void)
  {
    keytokey_.terminate();
    consumertoconsumer_.terminate();
    if (toKeys_) {
      delete toKeys_;
      toKeys_ = NULL;
    }
  }

  void
  RemapUtil::ConsumerToKey::add(ConsumerKeyCode newval)
  {
    switch (index_) {
      case 0:
        fromKey_.key = newval;
        consumertoconsumer_.add(newval);
        consumertoconsumer_.add(ConsumerKeyCode::VK_NONE);
        break;
      default:
        IOLOG_ERROR("Invalid ConsumerToKey::add\n");
        break;
    }
    ++index_;
  }

  void
  RemapUtil::ConsumerToKey::add(KeyCode newval)
  {
    switch (index_) {
      case 0:
        IOLOG_ERROR("Invalid ConsumerToKey::add\n");
        break;
      default:
        if (toKeys_) {
          toKeys_->push_back(PairKeyFlags(newval));
        }
        break;
    }
    ++index_;
  }

  void
  RemapUtil::ConsumerToKey::add(Flags newval)
  {
    switch (index_) {
      case 0:
        break;
      case 1:
        fromKey_.flags = newval;
        break;
      default:
        if (toKeys_) {
          toKeys_->back().flags = newval;
        }
        break;
    }
  }

  bool
  RemapUtil::ConsumerToKey::remap(RemapConsumerParams& remapParams)
  {
    if (! toKeys_) return false;

    if (! fromkeychecker_.isFromKey(remapParams, fromKey_.key, fromKey_.flags)) return false;

    bool result = consumertoconsumer_.remap(remapParams);
    if (! result) return false;

    // ----------------------------------------
    // XXX: A hack until KeyToKey being completed
    Params_KeyboardEventCallBack::auto_ptr ptr(Params_KeyboardEventCallBack::alloc(remapParams.params.eventType,
                                                                                   FlagStatus::makeFlags(),
                                                                                   KeyCode::VK_PSEUDO_KEY,
                                                                                   CommonData::getcurrent_keyboardType(),
                                                                                   false));
    if (! ptr) return false;
    Params_KeyboardEventCallBack& params = *ptr;

    RemapParams rp(params);
    if (! keytokey_.remap(rp, KeyCode::VK_PSEUDO_KEY, fromKey_.flags, (*toKeys_)[0].key, (*toKeys_)[0].flags)) {
      return false;
    }

    return true;
  }

  // ======================================================================
  void
  RemapUtil::ConsumerToConsumer::initialize(void)
  {
    toKeys_ = new Vector_PairConsumerKeyFlags();
  }

  void
  RemapUtil::ConsumerToConsumer::terminate(void)
  {
    if (toKeys_) {
      delete toKeys_;
      toKeys_ = NULL;
    }
  }

  void
  RemapUtil::ConsumerToConsumer::add(ConsumerKeyCode newval)
  {
    switch (index_) {
      case 0:
        fromKey_.key = newval;
        break;
      default:
        if (toKeys_) {
          toKeys_->push_back(PairConsumerKeyFlags(newval));
        }
        break;
    }
    ++index_;
  }

  void
  RemapUtil::ConsumerToConsumer::add(Flags newval)
  {
    switch (index_) {
      case 0:
        IOLOG_ERROR("Invalid ConsumerToConsumer::add\n");
        break;
      case 1:
        fromKey_.flags = newval;
        break;
      default:
        if (toKeys_) {
          toKeys_->back().flags = newval;
        }
        break;
    }
  }

  bool
  RemapUtil::ConsumerToConsumer::remap(RemapConsumerParams& remapParams)
  {
    if (! toKeys_) return false;

    // ------------------------------------------------------------
    // NumLock Hack
    // If we change NumLock key, we need to call IOHIKeyboard::setNumLock(false).
    // Unless call setNumLock, internal NumLock status of IOHIKeyboard is still active.
    // And NumLock retains working status.
    if (fromKey_.key == ConsumerKeyCode::NUMLOCK) {
      bool tonumlock = false;
      for (size_t i = 0; i < toKeys_->size(); ++i) {
        if ((*toKeys_)[i].key == ConsumerKeyCode::NUMLOCK) {
          tonumlock = true;
        }
      }
      if (! tonumlock) {
        HookedKeyboard* hk = ListHookedKeyboard::instance().get();
        if (hk) {
          IOHIKeyboard* kbd = hk->get();
          if (kbd) {
            if (kbd->numLock()) {
              kbd->setNumLock(false);
            }
          }
        }
      }
    }

    // ------------------------------------------------------------
    if (remapParams.isremapped) return false;
    if (! fromkeychecker_.isFromKey(remapParams, fromKey_.key, fromKey_.flags)) return false;
    remapParams.isremapped = true;

    // ----------------------------------------
    FlagStatus::temporary_decrease(fromKey_.flags);
    KeyboardRepeat::cancel();

    switch (toKeys_->size()) {
      case 0:
        break;

      case 1:
      {
        FlagStatus::temporary_increase((*toKeys_)[0].flags);
        Params_KeyboardSpecialEventCallback::auto_ptr ptr(Params_KeyboardSpecialEventCallback::alloc(remapParams.params.eventType,
                                                                                                     FlagStatus::makeFlags(),
                                                                                                     (*toKeys_)[0].key,
                                                                                                     false));
        if (! ptr) return false;
        Params_KeyboardSpecialEventCallback& params = *ptr;

        KeyboardRepeat::set(params);
        EventOutput::FireConsumer::fire(params);
        break;
      }

      default:
        if (remapParams.params.eventType == EventType::DOWN) {
          for (size_t i = 0; i < toKeys_->size(); ++i) {
            FlagStatus::temporary_increase((*toKeys_)[i].flags);
            Params_KeyboardSpecialEventCallback::auto_ptr ptr(Params_KeyboardSpecialEventCallback::alloc(EventType::DOWN,
                                                                                                         FlagStatus::makeFlags(),
                                                                                                         (*toKeys_)[i].key,
                                                                                                         false));
            if (! ptr) return false;
            Params_KeyboardSpecialEventCallback& params = *ptr;

            EventOutput::FireConsumer::fire(params);
            params.eventType = EventType::UP;
            EventOutput::FireConsumer::fire(params);

            KeyboardRepeat::primitive_add(EventType::DOWN, params.flags, params.key);
            KeyboardRepeat::primitive_add(EventType::UP,   params.flags, params.key);

            FlagStatus::temporary_decrease((*toKeys_)[i].flags);
          }

          KeyboardRepeat::primitive_start();
        }

        break;
    }

    return true;
  }

  void
  RemapUtil::KeyToConsumer::initialize(void)
  {
    keytokey_.initialize();
    consumertoconsumer_.initialize();
  }

  void
  RemapUtil::KeyToConsumer::terminate(void)
  {
    keytokey_.terminate();
    consumertoconsumer_.terminate();
  }

  void
  RemapUtil::KeyToConsumer::add(KeyCode newval)
  {
    switch (index_) {
      case 0:
        fromKey_.key = newval;
        break;
      default:
        IOLOG_ERROR("Invalid KeyToConsumer::add\n");
        break;
    }
    ++index_;
  }

  void
  RemapUtil::KeyToConsumer::add(ConsumerKeyCode newval)
  {
    switch (index_) {
      case 0:
        IOLOG_ERROR("Invalid KeyToConsumer::add\n");
        break;
      case 1:
        consumertoconsumer_.add(ConsumerKeyCode::VK_PSEUDO_KEY);
        consumertoconsumer_.add(fromKey_.flags);
        // pass-through
      default:
        consumertoconsumer_.add(newval);
        break;
    }
    ++index_;
  }

  void
  RemapUtil::KeyToConsumer::add(Flags newval)
  {
    switch (index_) {
      case 0:
        IOLOG_ERROR("Invalid KeyToConsumer::add\n");
        break;
      case 1:
        fromKey_.flags = newval;
        break;
      default:
        consumertoconsumer_.add(newval);
        break;
    }
  }

  bool
  RemapUtil::KeyToConsumer::remap(RemapParams& remapParams)
  {
    if (! fromkeychecker_.isFromKey(remapParams, fromKey_.key, fromKey_.flags)) return false;

    bool isKeyDown = remapParams.isKeyDownOrModifierDown();
    bool result = keytokey_.remap(remapParams, fromKey_.key, KeyCode::VK_NONE);
    if (! result) return false;

    // ----------------------------------------
    Params_KeyboardSpecialEventCallback::auto_ptr ptr(Params_KeyboardSpecialEventCallback::alloc(isKeyDown ? EventType::DOWN : EventType::UP,
                                                                                                 FlagStatus::makeFlags(),
                                                                                                 ConsumerKeyCode::VK_PSEUDO_KEY,
                                                                                                 false));
    if (! ptr) return false;
    Params_KeyboardSpecialEventCallback& params = *ptr;

    RemapConsumerParams rp(params);
    return consumertoconsumer_.remap(rp);
  }

  bool
  RemapUtil::PointingButtonToPointingButton::remap(RemapPointingParams_relative& remapParams,
                                                   PointingButton fromButton, Flags fromFlags,
                                                   PointingButton toButton,   Flags toFlags)
  {
    if (remapParams.isremapped) return false;

    // We consider it about Option_L+LeftClick to MiddleClick.
    // LeftClick generates the following events by ButtonDown and ButtonUp.
    //
    // (1) buttons == PointingButton::LEFT  (ButtonDown event)
    // (2) buttons == PointingButton::NONE  (ButtonUp event)
    //
    // We must cancel Option_L in both (1), (2).
    // We use "active_" flag to detect (2), because the "buttons" has no useful information at (2).
    //
    // Attention: We need fire MiddleClick only at (1).

    if (ButtonStatus::justPressed().isOn(fromButton) && FlagStatus::makeFlags().isOn(fromFlags)) {
      active_ = true;
      ButtonStatus::decrease(fromButton);
      ButtonStatus::increase(toButton);
    } else if (ButtonStatus::justReleased().isOn(fromButton) && active_) {
      // We need to handle ButtonUp Event, so keep active_.
      ButtonStatus::decrease(toButton);
      ButtonStatus::increase(fromButton);
    }

    if (! active_) return false;

    // ----------------------------------------
    FlagStatus::temporary_decrease(fromFlags);
    FlagStatus::temporary_increase(toFlags);

    EventOutput::FireRelativePointer::fire(ButtonStatus::makeButtons(), remapParams.params.dx, remapParams.params.dy);

    if (ButtonStatus::justReleased().isOn(fromButton)) {
      active_ = false;
    }

    remapParams.drop();
    return true;
  }

  bool
  RemapUtil::KeyToPointingButton::remap(RemapParams& remapParams, KeyCode fromKeyCode, Flags fromFlags, PointingButton toButton)
  {
    if (remapParams.isremapped) return false;
    if (remapParams.params.key != fromKeyCode) return false;

    bool isKeyDown = remapParams.isKeyDownOrModifierDown();
    if (isKeyDown) {
      if (! FlagStatus::makeFlags().isOn(fromFlags)) return false;
      active_ = true;
    } else {
      if (! active_) return false;
      active_ = false;
    }

    // ------------------------------------------------------------
    remapParams.isremapped = true;

    // We need to call FlagStatus::decrease before mapping,
    // and call FlagStatus::increase after a mapping.
    //
    // ex. Option_L+Space to Right Button
    // (1) KeyDown Option_L
    //      1. KeyDown Option_L
    // (2) KeyDown Space
    //      2. KeyUp   Option_L
    //      3. ButtonDown Right
    // (3) KeyUp   Space
    //      4. ButtonUp   Right
    //      5. KeyUp   Option_L
    //
    // *** To support Mouse Dragging,
    // *** we need to use FlagStatus::decrease/increase.
    // *** (not temporary_decrease/temporary_increase).

    if (isKeyDown) {
      FlagStatus::decrease(fromFlags | fromKeyCode.getModifierFlag());
      ButtonStatus::increase(toButton);
      EventOutput::FireRelativePointer::fire();

    } else {
      ButtonStatus::decrease(toButton);
      EventOutput::FireRelativePointer::fire();

      FlagStatus::increase(fromFlags | fromKeyCode.getModifierFlag());
      EventOutput::FireModifiers::fire();
    }

    return true;
  }

  bool
  RemapUtil::PointingButtonToKey::remap(RemapPointingParams_relative& remapParams,
                                        PointingButton fromButton, Flags fromFlags,
                                        KeyCode toKeyCode1,  Flags toFlags1,
                                        KeyCode toKeyCode2,  Flags toFlags2,
                                        KeyCode toKeyCode3,  Flags toFlags3,
                                        KeyCode toKeyCode4,  Flags toFlags4,
                                        KeyCode toKeyCode5,  Flags toFlags5)
  {
    if (remapParams.isremapped) return false;

    if (ButtonStatus::justPressed().isOn(fromButton)) {
      if (FlagStatus::makeFlags().isOn(fromFlags)) {
        ButtonStatus::decrease(fromButton);
        active_ = true;
        goto doremap;
      }
    } else if (ButtonStatus::justReleased().isOn(fromButton)) {
      if (active_) {
        ButtonStatus::increase(fromButton);
        active_ = false;
        goto doremap;
      }
    }

    return false;

  doremap:
    // ----------------------------------------
    Params_KeyboardEventCallBack::auto_ptr ptr(Params_KeyboardEventCallBack::alloc(active_ ? EventType::DOWN : EventType::UP,
                                                                                   FlagStatus::makeFlags(),
                                                                                   KeyCode::VK_PSEUDO_KEY,
                                                                                   CommonData::getcurrent_keyboardType(),
                                                                                   false));
    if (! ptr) return false;
    Params_KeyboardEventCallBack& params = *ptr;

    RemapParams rp(params);
    if (toKeyCode2 == KeyCode::VK_NONE) {
      if (! keytokey_.remap(rp, KeyCode::VK_PSEUDO_KEY, fromFlags, toKeyCode1, toFlags1)) {
        return false;
      }
    } else {
      if (! keytokey_.remap(rp, KeyCode::VK_PSEUDO_KEY, fromFlags,
                            toKeyCode1, toFlags1,
                            toKeyCode2, toFlags2,
                            toKeyCode3, toFlags3,
                            toKeyCode4, toFlags4,
                            toKeyCode5, toFlags5)) {
        return false;
      }
    }

    remapParams.drop();
    return true;
  }

  // ------------------------------------------------------------
  bool
  RemapUtil::PointingRelativeToScroll::remap(RemapPointingParams_relative& remapParams, Buttons buttons, Flags fromFlags)
  {
    if (! FlagStatus::makeFlags().isOn(fromFlags)) return false;

    if (buttons == PointingButton::NONE) {
      FlagStatus::temporary_decrease(fromFlags);
      toscroll(remapParams);
      return true;
    }

    if (ButtonStatus::makeButtons().isOn(buttons)) {
      // if the source buttons contains left button, we cancel left click for iPhoto, or some applications.
      // iPhoto store the scroll events when left button is pressed, and restore events after left button is released.
      // PointingRelativeToScroll doesn't aim it, we release the left button and do normal scroll event.
      if (! active_) {
        ButtonStatus::decrease(buttons);
        EventOutput::FireRelativePointer::fire();
        ButtonStatus::increase(buttons);

        absolute_distance_ = 0;
        begin_ic_.begin();
        buffered_delta1 = 0;
        buffered_delta2 = 0;
      }

      active_ = true;
      FlagStatus::temporary_decrease(fromFlags);
      toscroll(remapParams);

      return true;

    } else {
      // ignore button up event.
      if (active_) {
        active_ = false;
        remapParams.isremapped = true;

        const uint32_t DISTANCE_THRESHOLD = 5;
        const uint32_t TIME_THRESHOLD = 300;
        if (absolute_distance_ <= DISTANCE_THRESHOLD && begin_ic_.getmillisec() < TIME_THRESHOLD) {
          // Fire by a click event.
          ButtonStatus::increase(buttons);
          EventOutput::FireRelativePointer::fire();
          ButtonStatus::decrease(buttons);
          EventOutput::FireRelativePointer::fire();
        }

        return true;
      }

      return false;
    }
  }

  void
  RemapUtil::PointingRelativeToScroll::toscroll(RemapPointingParams_relative& remapParams)
  {
    remapParams.isremapped = true;

    // ----------------------------------------
    // Buffer processing

    // buffer events in 20ms (60fps)
    const uint32_t BUFFER_MILLISEC = 20;

    buffered_delta1 += -remapParams.params.dy;
    buffered_delta2 += -remapParams.params.dx;

    if (buffered_ic_.getmillisec() < BUFFER_MILLISEC) {
      return;
    }

    int delta1 = buffered_delta1;
    int delta2 = buffered_delta2;
    buffered_delta1 = 0;
    buffered_delta2 = 0;
    buffered_ic_.begin();

    // ----------------------------------------
    if (config.option_pointing_disable_vertical_scroll) delta1 = 0;
    if (config.option_pointing_disable_horizontal_scroll) delta2 = 0;

    // ----------------------------------------
    // ignore minuscule move
    const unsigned int abs1 = RemapUtil::abs(delta1);
    const unsigned int abs2 = RemapUtil::abs(delta2);

    if (abs1 > abs2 * 2) {
      delta2 = 0;
    }
    if (abs2 > abs1 * 2) {
      delta1 = 0;
    }

    // ----------------------------------------
    // Fixation processing

    if (config.option_pointing_enable_scrollwheel_fixation) {
      // When 300ms passes from the last event, we reset a value.
      const uint32_t FIXATION_MILLISEC = 300;
      if (fixation_ic_.getmillisec() > FIXATION_MILLISEC) {
        fixation_begin_ic_.begin();
        fixation_delta1 = 0;
        fixation_delta2 = 0;
      }
      fixation_ic_.begin();

      if (fixation_delta1 > fixation_delta2 * 2) {
        delta2 = 0;
      }
      if (fixation_delta2 > fixation_delta1 * 2) {
        delta1 = 0;
      }

      // Only first 1000ms performs the addition of fixation_delta1, fixation_delta2.
      const uint32_t FIXATION_EARLY_MILLISEC  = 1000;
      if (fixation_begin_ic_.getmillisec() < FIXATION_EARLY_MILLISEC) {
        if (delta1 == 0) fixation_delta2 += abs2;
        if (delta2 == 0) fixation_delta1 += abs1;
      }
    }

    // ----------------------------------------
    if (delta1 == 0 && delta2 == 0) return;

    Params_ScrollWheelEventCallback::auto_ptr ptr(Params_ScrollWheelEventCallback::alloc(0, 0, 0,
                                                                                         0, 0, 0,
                                                                                         0, 0, 0,
                                                                                         0));
    if (! ptr) return;
    Params_ScrollWheelEventCallback& params = *ptr;

    params.deltaAxis1 = (delta1 * config.pointing_relative2scroll_rate) / 1024;
    if (params.deltaAxis1 == 0 && delta1 != 0) {
      params.deltaAxis1 = delta1 > 0 ? 1 : -1;
    }
    params.deltaAxis2 = (delta2 * config.pointing_relative2scroll_rate) / 1024;
    if (params.deltaAxis2 == 0 && delta2 != 0) {
      params.deltaAxis2 = delta2 > 0 ? 1 : -1;
    }

    // ----------------------------------------
    params.fixedDelta1 = (delta1 * config.pointing_relative2scroll_rate) * (POINTING_FIXED_SCALE / 1024);
    params.fixedDelta2 = (delta2 * config.pointing_relative2scroll_rate) * (POINTING_FIXED_SCALE / 1024);

    params.pointDelta1 = (delta1 * POINTING_POINT_SCALE * config.pointing_relative2scroll_rate) / 1024;
    params.pointDelta2 = (delta2 * POINTING_POINT_SCALE * config.pointing_relative2scroll_rate) / 1024;

    EventOutput::FireScrollWheel::fire(params);

    absolute_distance_ += RemapUtil::abs(delta1) + RemapUtil::abs(delta2);
  }

  // ----------------------------------------------------------------------
  bool
  KeyOverlaidModifier::remap(RemapParams& remapParams,
                             KeyCode fromKeyCode,   Flags fromFlags,
                             KeyCode toKeyCode,     Flags toFlags,
                             KeyCode fireKeyCode1,  Flags fireFlags1,
                             KeyCode fireKeyCode2,  Flags fireFlags2,
                             KeyCode fireKeyCode3,  Flags fireFlags3,
                             KeyCode fireKeyCode4,  Flags fireFlags4,
                             KeyCode fireKeyCode5,  Flags fireFlags5,
                             bool isFireRepeat)
  {
    // ----------------------------------------
    bool isKeyDown = remapParams.isKeyDownOrModifierDown();
    bool savedIsAnyEventHappen = isAnyEventHappen_;

    if (! keytokey_.remap(remapParams, fromKeyCode, fromFlags, toKeyCode, toFlags)) {
      return false;
    }

    // ----------------------------------------
    if (isKeyDown) {
      EventWatcher::set(isAnyEventHappen_);
      ic_.begin();

      // ----------------------------------------
      // We store the flags when KeyDown.
      // Because it lets you make a natural input when the following sequence.
      //
      // ex. "Space to Shift (when type only, send Space)"
      // (1) KeyDown Command_L
      // (2) KeyDown Space        save flags (Command_L)
      // (3) KeyUp   Command_L
      // (4) KeyUp   Space        fire Command_L+Space

      // calc flags
      ModifierFlag toKeyCodeFlag = toKeyCode.getModifierFlag();
      FlagStatus::temporary_decrease(toFlags | toKeyCodeFlag);

      savedflags_ = FlagStatus::makeFlags();

      // restore flags
      FlagStatus::temporary_increase(toFlags | toKeyCodeFlag);

      // ----------------------------------------
      if (isFireRepeat) {
        KeyboardRepeat::cancel();

        KeyboardRepeat::primitive_add(EventType::DOWN, (fireFlags1 | savedflags_).stripNONE(), fireKeyCode1, remapParams.params.keyboardType);
        KeyboardRepeat::primitive_add(EventType::UP,   (fireFlags1 | savedflags_).stripNONE(), fireKeyCode1, remapParams.params.keyboardType);
        KeyboardRepeat::primitive_add(EventType::DOWN, (fireFlags2 | savedflags_).stripNONE(), fireKeyCode2, remapParams.params.keyboardType);
        KeyboardRepeat::primitive_add(EventType::UP,   (fireFlags2 | savedflags_).stripNONE(), fireKeyCode2, remapParams.params.keyboardType);
        KeyboardRepeat::primitive_add(EventType::DOWN, (fireFlags3 | savedflags_).stripNONE(), fireKeyCode3, remapParams.params.keyboardType);
        KeyboardRepeat::primitive_add(EventType::UP,   (fireFlags3 | savedflags_).stripNONE(), fireKeyCode3, remapParams.params.keyboardType);
        KeyboardRepeat::primitive_add(EventType::DOWN, (fireFlags4 | savedflags_).stripNONE(), fireKeyCode4, remapParams.params.keyboardType);
        KeyboardRepeat::primitive_add(EventType::UP,   (fireFlags4 | savedflags_).stripNONE(), fireKeyCode4, remapParams.params.keyboardType);
        KeyboardRepeat::primitive_add(EventType::DOWN, (fireFlags5 | savedflags_).stripNONE(), fireKeyCode5, remapParams.params.keyboardType);
        KeyboardRepeat::primitive_add(EventType::UP,   (fireFlags5 | savedflags_).stripNONE(), fireKeyCode5, remapParams.params.keyboardType);

        KeyboardRepeat::primitive_start(config.get_keyoverlaidmodifier_initial_wait());
      }

    } else {
      if (savedIsAnyEventHappen == false) {
        if (config.parameter_keyoverlaidmodifier_timeout <= 0 || ic_.checkThreshold(config.parameter_keyoverlaidmodifier_timeout) == false) {
          EventOutput::FireKey::fire_downup((fireFlags1 | savedflags_).stripNONE(), fireKeyCode1, remapParams.params.keyboardType);
          EventOutput::FireKey::fire_downup((fireFlags2 | savedflags_).stripNONE(), fireKeyCode2, remapParams.params.keyboardType);
          EventOutput::FireKey::fire_downup((fireFlags3 | savedflags_).stripNONE(), fireKeyCode3, remapParams.params.keyboardType);
          EventOutput::FireKey::fire_downup((fireFlags4 | savedflags_).stripNONE(), fireKeyCode4, remapParams.params.keyboardType);
          EventOutput::FireKey::fire_downup((fireFlags5 | savedflags_).stripNONE(), fireKeyCode5, remapParams.params.keyboardType);
        }
      }
      EventWatcher::unset(isAnyEventHappen_);
    }

    return true;
  }

  // ----------------------------------------------------------------------
  bool
  DoublePressModifier::remap(RemapParams& remapParams, KeyCode fromKeyCode, KeyCode toKeyCode, KeyCode fireKeyCode, Flags fireFlags)
  {
    if (remapParams.isremapped || remapParams.params.key != fromKeyCode) {
      pressCount_ = 0;
      return false;
    }

    // ----------------------------------------
    bool isKeyDown = remapParams.isKeyDownOrModifierDown();

    keytokey_.remap(remapParams, fromKeyCode, toKeyCode);

    if (ic_.getmillisec() > static_cast<uint32_t>(config.parameter_doublepressmodifier_threshold)) {
      pressCount_ = 0;
    }
    ic_.begin();

    if (isKeyDown) {
      ++pressCount_;
    } else {
      if (pressCount_ >= 2) {
        pressCount_ = 0;
        Flags flags = (FlagStatus::makeFlags() | fireFlags).stripNONE();
        EventOutput::FireKey::fire_downup(flags, fireKeyCode, remapParams.params.keyboardType);
      }
    }

    return true;
  }

  // ----------------------------------------
  bool
  ModifierHoldingKeyToKey::remap(RemapParams& remapParams, KeyCode fromKeyCode, Flags fromFlags, KeyCode toKeyCode)
  {
    if (remapParams.isremapped || remapParams.params.key != fromKeyCode) {
      goto nottargetkey;
    }

    if (! FlagStatus::makeFlags().isOn(fromFlags)) goto nottargetkey;
    if (! ic_.checkThreshold(config.parameter_modifierholdingkeytokey_wait)) goto nottargetkey;

    return keytokey_.remap(remapParams, fromKeyCode, fromFlags, toKeyCode);

  nottargetkey:
    ic_.begin();
    return false;
  }

  // ----------------------------------------
  TimerWrapper HoldingKeyToKey::timer_;
  KeyCode HoldingKeyToKey::toKeyCode_holding_;
  Flags HoldingKeyToKey::toFlags_holding_;
  bool HoldingKeyToKey::isfireholding_;
  bool HoldingKeyToKey::isfirenormal_;

  void
  HoldingKeyToKey::initialize(IOWorkLoop& workloop)
  {
    timer_.initialize(&workloop, NULL, HoldingKeyToKey::fireholding);
  }

  void
  HoldingKeyToKey::terminate(void)
  {
    timer_.terminate();
  }

  bool
  HoldingKeyToKey::remap(RemapParams& remapParams, KeyCode fromKeyCode, Flags fromFlags,
                         KeyCode toKeyCode_normal,  Flags toFlags_normal,
                         KeyCode toKeyCode_holding, Flags toFlags_holding)
  {
    IOLockWrapper::ScopedLock lk(timer_.getlock());

    bool isKeyDown = remapParams.isKeyDownOrModifierDown();

    bool result = keytokey_.remap(remapParams, fromKeyCode, fromFlags, KeyCode::VK_NONE);
    if (! result) return false;

    if (isKeyDown) {
      isfirenormal_ = false;
      isfireholding_ = false;
      toKeyCode_holding_ = toKeyCode_holding;
      toFlags_holding_ = toFlags_holding;
      timer_.setTimeoutMS(config.get_holdingkeytokey_wait());
    } else {
      timer_.cancelTimeout();
      if (! isfireholding_) {
        isfirenormal_ = true;
        ModifierFlag toKeyCodeFlag = toKeyCode_normal.getModifierFlag();
        FlagStatus::temporary_decrease(toFlags_normal | toKeyCodeFlag);
        EventOutput::FireKey::fire_downup(toFlags_normal.stripNONE(), toKeyCode_normal, CommonData::getcurrent_keyboardType());
        FlagStatus::temporary_increase(toFlags_normal | toKeyCodeFlag);
      }
    }
    return true;
  }

  void
  HoldingKeyToKey::fireholding(OSObject* owner, IOTimerEventSource* sender)
  {
    IOLockWrapper::ScopedLock lk(timer_.getlock());

    if (! isfirenormal_) {
      isfireholding_ = true;
      EventOutput::FireKey::fire_downup(toFlags_holding_.stripNONE(), toKeyCode_holding_, CommonData::getcurrent_keyboardType());
    }
  }
}
