#pragma once

#include <utility>    // std::exchange
#include <cstddef>    // nullptr_t

namespace Game {

  // 控制區塊，用於管理物件指標與存活旗標
  template<typename T>
  struct ControlBlock {
    // 原始物件指標
    T* ptr;
    // 存活旗標
    struct WeakFlag {
      bool alive;
    } flag;

    // 建構：接收物件指標，並將 flag.alive 設為 true
    explicit ControlBlock(T* p) noexcept;

    // 解構：若 ptr 未釋放則刪除，並將 flag.alive 設為 false
    ~ControlBlock();
  };

  // 前向宣告 WeakRef，以供 UniqueWithWeak 使用
  template<typename T>
  class WeakRef;

  // 唯一擁有者，且能衍生弱參照
  template<typename T>
  class UniqueWithWeak {
  public:
    // 建構：直接接收原生指標，建立 ControlBlock
    explicit UniqueWithWeak(T* ptr) noexcept;

    // 移動建構：接管對方的 control block
    UniqueWithWeak(UniqueWithWeak&& other) noexcept;

    // 移動指派：釋放自身後接管對方 control block
    UniqueWithWeak& operator=(UniqueWithWeak&& other) noexcept;

    // 解構：釋放管理的物件與控制區塊
    ~UniqueWithWeak();

    // 取得原生指標
    T* get() const noexcept;

    // 便捷存取成員函式
    T* operator->() const noexcept;

    // 放棄擁有權，回傳原生指標並置空控制區塊
    T* release() noexcept;

    // 重置為新的原生指標，舊物件會被刪除
    void reset(T* ptr) noexcept;

    // 取得對應的弱參照
    WeakRef<T> getWeak() const noexcept;

  private:
    ControlBlock<T>* ctrl_;
  };

  // 弱參照，不持有物件，只能查詢與鎖定
  template<typename T>
  class WeakRef {
  public:
    // 預設建構：無任何 control block
    WeakRef() noexcept;

    // 從另一弱參照複製
    WeakRef(const WeakRef<T>& other) noexcept;

    // 弱參照指派
    WeakRef& operator=(const WeakRef<T>& other) noexcept;

    // 查詢物件是否已被銷毀
    bool expired() const noexcept;

    // 若物件仍存活，回傳原生指標；否則回傳 nullptr
    T* lock() const noexcept;

  private:
    ControlBlock<T>* ctrl_;
  };

} // namespace Game
