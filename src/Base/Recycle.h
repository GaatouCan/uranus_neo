#pragma once

#include "../Common.h"

#include <memory>
#ifdef __linux__
#include <cstdint>
#endif

using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;
using std::make_unique;


/**
 * The Interface Of Recyclable Object,
 * The Element Managed By Recycler
 */
class BASE_API IRecycleInterface {

    friend class IRecyclerBase;

protected:
    /** Called When It Created By Recycler */
    virtual void OnCreate() {}

    /** Called When Acquiring From Recycler */
    virtual void Initial() = 0;

    /** Called When It Recycled To Recycler */
    virtual void Reset() = 0;

public:
    IRecycleInterface() = default;
    virtual ~IRecycleInterface() = default;

    DISABLE_COPY_MOVE(IRecycleInterface)

    /** Depth Copy Object Data */
    virtual bool CopyFrom(IRecycleInterface *other);

    /** Depth Copy Object Data */
    virtual bool CopyFrom(const std::shared_ptr<IRecycleInterface> &other);

    /** After Acquiring And Before Assigning Return True */
    [[nodiscard]] virtual bool IsUnused() const = 0;

    /** After Recycling It Was False */
    [[nodiscard]] virtual bool IsAvailable() const = 0;
};
