#pragma once
#include <atomic>
#include <cstdint>
#include <cassert>

// Same as IUnknown, internal ref count is thread safe
class RefCounter
{
public:
    RefCounter() : NumRefs(0) {}
    virtual ~RefCounter() {assert(NumRefs == 0);}
    RefCounter(const RefCounter&) = delete;
    RefCounter operator=(const RefCounter&) = delete;
	RefCounter(RefCounter&&)  = delete;
	RefCounter& operator=(const RefCounter&&)  = delete;

    uint32_t AddRef()
    {
        return ++NumRefs;
    }

    uint32_t Release()
    {
        uint32_t Refs = --NumRefs;
        if (Refs == 0)
        {
            delete this;
        }
        return Refs;
    }

	uint32_t GetRefCount() const
	{
		return NumRefs;
	}
    
private:
    std::atomic<uint32_t> NumRefs;
};

// Same as ComPtr, Copy from UE5 RefCountPtr
template<typename ReferencedType>
class RefCountPtr
{
typedef ReferencedType* ReferenceType;

public:

	RefCountPtr():
		Reference(nullptr)
	{ }

	RefCountPtr(ReferencedType* InReference,bool bAddRef = true)
	{
		Reference = InReference;
		if(Reference && bAddRef)
		{
			Reference->AddRef();
		}
	}

	RefCountPtr(const RefCountPtr& Copy)
	{
		Reference = Copy.Reference;
		if(Reference)
		{
			Reference->AddRef();
		}
	}

	template<typename CopyReferencedType>
	explicit RefCountPtr(const RefCountPtr<CopyReferencedType>& Copy)
	{
		Reference = static_cast<ReferencedType*>(Copy.GetReference());
		if (Reference)
		{
			Reference->AddRef();
		}
	}

	RefCountPtr(RefCountPtr&& Move)
	{
		Reference = Move.Reference;
		Move.Reference = nullptr;
	}

	template<typename MoveReferencedType>
	explicit RefCountPtr(RefCountPtr<MoveReferencedType>&& Move)
	{
		Reference = static_cast<ReferencedType*>(Move.GetReference());
		Move.Reference = nullptr;
	}

	~RefCountPtr()
	{
		if(Reference)
		{
			Reference->Release();
		}
	}

	RefCountPtr& operator=(ReferencedType* InReference)
	{
		if (Reference != InReference)
		{
			// Call AddRef before Release, in case the new reference is the same as the old reference.
			ReferencedType* OldReference = Reference;
			Reference = InReference;
			if (Reference)
			{
				Reference->AddRef();
			}
			if (OldReference)
			{
				OldReference->Release();
			}
		}
		return *this;
	}

	RefCountPtr& operator=(const RefCountPtr& InPtr)
	{
		return *this = InPtr.Reference;
	}

	template<typename CopyReferencedType>
	RefCountPtr& operator=(const RefCountPtr<CopyReferencedType>& InPtr)
	{
		return *this = InPtr.GetReference();
	}

	RefCountPtr& operator=(RefCountPtr&& InPtr)
	{
		if (this != &InPtr)
		{
			ReferencedType* OldReference = Reference;
			Reference = InPtr.Reference;
			InPtr.Reference = nullptr;
			if(OldReference)
			{
				OldReference->Release();
			}
		}
		return *this;
	}

	template<typename MoveReferencedType>
	RefCountPtr& operator=(RefCountPtr<MoveReferencedType>&& InPtr)
	{
		// InPtr is a different type (or we would have called the other operator), so we need not test &InPtr != this
		ReferencedType* OldReference = Reference;
		Reference = InPtr.Reference;
		InPtr.Reference = nullptr;
		if (OldReference)
		{
			OldReference->Release();
		}
		return *this;
	}

	ReferencedType* operator->() const
	{
		return Reference;
	}

	operator ReferenceType() const
	{
		return Reference;
	}
	
	ReferencedType** GetInitReference()
	{
		*this = nullptr;
		return &Reference;
	}

	ReferencedType* GetReference() const
	{
		return Reference;
	}

	ReferencedType** GetReferenceAddress()
	{
		return &Reference;
	}

	friend bool IsValidRef(const RefCountPtr& InReference)
	{
		return InReference.Reference != nullptr;
	}

	bool IsValid() const
	{
		return Reference != nullptr;
	}

	// Same as ComPtr::Reset
	void SafeRelease()
	{
		*this = nullptr;
	}

	uint32_t GetRefCount()
	{
		uint32_t Result = 0;
		if (Reference)
		{
			Result = Reference->GetRefCount();
			assert(Result > 0); // you should never have a zero ref count if there is a live ref counted pointer (*this is live)
		}
		return Result;
	}

	void Swap(RefCountPtr& InPtr) // this does not change the reference count, and so is faster
	{
		ReferencedType* OldReference = Reference;
		Reference = InPtr.Reference;
		InPtr.Reference = OldReference;
	}

private:

	ReferencedType* Reference;

	template <typename OtherType>
	friend class RefCountPtr;
    
};
