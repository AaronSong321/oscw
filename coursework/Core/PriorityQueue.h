#pragma once

#include "Memory.h"
#include "Exception.h"
#include "Collection.h"
#include "IntRange.h"
#include "Comparator.h"

namespace Commons
{
    template<class T>
    struct FibonacciNode;
    template<class T>
    class FibonacciHeap;
    template<class T>
    struct FibonacciNodeChildrenEnumerator;

    template<class TKey>
    struct FibonacciNode: public IComparable<FibonacciNode<TKey>>
    {
    private:
        TKey _key;
        SharedPointer<FibonacciNode<TKey>> _left;
        SharedPointer<FibonacciNode<TKey>> _right;
        SharedPointer<FibonacciNode<TKey>> _child;
        SharedPointer<FibonacciNode<TKey>> _parent;
        int _degree;
        bool _mark;
        bool _minimumCausedByDeletion;
        SharedPointer<FibonacciHeap<TKey>> _tree;

        friend class FibonacciHeap<TKey>;
        friend class FibonacciNodeChildrenEnumerator<TKey>;

        void AddChild(SharedPointer<FibonacciNode<TKey>> node) {
            ThrowIf0(node);
            if (!CompareNode(*this, *node))
                ShallThrow("Cannot add this as a child");
            if (!_child){
                _child=node;
                node->_left=node;
                node->_right=node;
            }
            else if (_child->_right==_child){
                _child->_right=node;
                _child->_left=node;
                node->right=_child;
                node->left=_child;
            }
            else{
                _child->_right->_left=node;
                node->_right=_child->_right;
                _child->_right=node;
                node->_left=_child;
            }
            node->_parent=*this;
            ++_degree;
        }

        void DeleteChild(SharedPointer<FibonacciNode<TKey>> node){
            ThrowIf0(node);
            if (!CompareNode(*this, *node))
                ShallThrow("Cannot add this as a child");
            if (_child->_left == _child && _child == node){
                _child=0;
            }
            else{
                node->_left->_right=node->_right;
                node->_right->_left=node->_left;
                if (_child==node)
                    _child=node->_right;
            }
            node->_parent=0;
            node->_left=0;
            node->_right=0;
            --_degree;
        }


    public:
        static int CompareNode(SharedPointer<FibonacciNode<TKey>> lhs, SharedPointer<FibonacciNode<TKey>> rhs) {
            ThrowIf0(lhs);
            ThrowIf0(rhs);
            if (lhs->_tree != rhs->_tree)
                ShallThrow(0);
            if (lhs->_minimumCausedByDeletion) {
                if (rhs->_minimumCausedByDeletion)
                    return 0;
                return -1;
            }
            if (rhs->_minimumCausedByDeletion)
                return 1;
            return lhs->_tree->CompareKey(lhs->GetKey(), rhs->GetKey());
        }

        using IComparable<FibonacciNode<TKey>>::CompareTo;
        virtual int CompareTo(FibonacciNode<TKey> other) const override {
            if (_tree != other._tree)
                ShallThrow(0);
            if (_minimumCausedByDeletion){
                if (other._minimumCausedByDeletion)
                    return 0;
                return -1;
            }
            if (other._minimumCausedByDeletion)
                return 1;
            return _tree->CompareKey(_key, other._key);
        }

        TKey GetKey() const {
            return _key;
        }
        FibonacciNode<TKey> GetChild() const { return _child; }
        FibonacciNode<TKey> GetLeft() const { return _left; }
        FibonacciNode<TKey> GetRight() const { return _right; }

        FibonacciNode(TKey key, SharedPointer<FibonacciHeap<TKey>> tree):_key(key),_left(0),_right(0),_child(0),_parent(0),_degree(0),_mark(false),_minimumCausedByDeletion(false),_tree(tree) {
        }
    };

    template <class TKey>
    SharedPointer<IEnumerable<FibonacciNode<TKey>>> GetChildren(SharedPointer<FibonacciNode<TKey>> node){
        auto src = EnumeratorToEnumerable(FibonacciNodeChildrenEnumerator(node));
        return SharedPointer(src).StaticCast<IEnumerable<FibonacciNode<TKey>>();
    }

    template <class TKey>
    class FibonacciNodeChildrenEnumerator: public Collections::IEnumerator<FibonacciNode<TKey>> {
    private:
        SharedPointer<FibonacciNode<TKey>> _iter;
        int _index;
        const int _degree;

    public:
        FibonacciNodeChildrenEnumerator(SharedPointer<FibonacciNode<TKey>> root): _iter(root->_child->_left), _index(0), _degree(root->_degree){

        }
        // using Collections::IEnumerator<TKey>::MoveNext;
        virtual bool MoveNext() override {
            if (_index++ == _degree)
                return false;
            _iter = _iter->_right;
            return true;
        }
        virtual SharedPointer<FibonacciNode<TKey>> Get() override {
            return _iter;
        }
    };


    template<class TKey>
    class FibonacciHeap: public Collections::IInputCollection<TKey>
    {
    private:
        SharedPointer<FibonacciNode<TKey>> _min;
        int _degree;
        const IComparator<TKey> _comparator;

        void AddToRootList(SharedPointer<FibonacciNode<TKey>> node){
            ThrowIf0(node);
            if (!_min){
                _min=node;
                node->_left=node->_right=node;
            } else{
                _min->_left->_right = node;
                node->_left=_min->_left;
                _min->_left=node;
                node->_right=_min;
            }
            node->_parent=0;
        }

        void RemoveFromRootList(SharedPointer<FibonacciNode<TKey>> node){
            ThrowIf0(node);
            if (_min->_left == _min){
                if (node != _min)
                    ShallThrow("Incorrect node removal");
                _min = 0;
            } else {
                node->_left->_right=node->_right;
                node->_right->_left=node->_left;
            }
            node->_parent = 0;
        }

        void Consolidate(){
            SharedPointer<FibonacciNode<TKey>> a[_degree+1];
            memset(a, 0, sizeof(a));
            auto w = _min;
            const auto initRoot = _min;
            do {
                auto x = w;
                w=w->_right;
                auto d = x->_degree;
                while (!a[d]){
                    auto y = a[d];
                    if (x > y){
                        auto t = x;
                        x = y;
                        y = t;
                    }
                    Link(y, x);
                    a[d] = 0;
                    ++d;
                }
                a[d] = x;
            } while (w != initRoot);
            _min = 0;
            Until<int>(0, _degree).ForEach([this, a](int i){
                if (!a[i]){
                    if (!_min){
                        AddToRootList(a[i]);
                        _min = a[i];
                    } else {
                        AddToRootList(a[i]);
                        if (FibonacciNode<TKey>::CompareNodea[i] < _min)
                            _min = a[i];
                    }
                }
            });
        }

        void Link(SharedPointer<FibonacciNode<TKey>> y, SharedPointer<FibonacciNode<TKey>> x){
            RemoveFromRootList(y);
            x->AddChild(y);
            y->_mark = false;
        }

        void Cut(SharedPointer<FibonacciNode<TKey>> x, SharedPointer<FibonacciNode<TKey>> y){
            y->DeleteChild(x);
            AddToRootList(x);
            x->_parent = 0;
            x->_mark = false;
        }

        void CascadingCut(SharedPointer<FibonacciNode<TKey>> y){
            auto z = y->_parent;
            if (!z){
                if (!y->_mark)
                    y->_mark = true;
                else{ // node y has lost more than two children before being cut last time
                    // therefore it's not balanced and need to be cut
                    Cut(y, z);
                    CascadingCut(z);
                }
            }
        }

    public:
        // using Collections::IInputCollection<TKey>::GetCount;
        // using Collections::IInputCollection<TKey>::Add;
        // using Collections::IInputCollection<TKey>::Clear;

        FibonacciHeap(IComparator<TKey> comparator): _min(0), _degree(0), _comparator(comparator) {
        }

        inline bool CompareKey(TKey lhs, TKey rhs) const {
            return _comparator(lhs, rhs);
        }

        SharedPointer<FibonacciNode<TKey>> Insert(TKey key){
            auto node = MakeShared<FibonacciNode<TKey>>(key, SharedPointer(this));
            if (!_min){
                AddToRootList(node);
                _min = node;
            } else {
                AddToRootList(node);
                if (FibonacciNode<TKey>::CompareNode(node, _min) < 0)
                    _min = node;
            }
            ++_degree;
            return node;
        }

        static SharedPointer<FibonacciHeap<TKey>> Union(SharedPointer<FibonacciHeap<TKey>> a, SharedPointer<FibonacciHeap<TKey>> b){
            ThrowIf0(a);
            ThrowIf0(b);
            if (a->_comparator != b->_comparator)
                ShallThrow("Argument trees are of different comparators");
            auto ans = MakeShared<FibonacciHeap<TKey>>(a->_comparator);
            ans._min = a->_min;
            for (auto child: *b){
                a->AddToRootList(child);
            }
            if (!a->_min || (!b->_min && a->CompareKey(*b->_min, *a->_min) < 0))
                ans->_min = b->_min;
            ans._degree = a._degree + b._degree;
            return ans;
        }

        FibonacciNode<TKey> ExtractMin(){
            auto z = _min;
            if (!z){
                GetChildren(z)->ForEach([this](SharedPointer<FibonacciNode<TKey>> node) { AddToRootList(node); node->_parent = 0; });
            }
            RemoveFromRootList(z);
            if (z == z->_right)
                _min = 0;
            else{
                _min = z->_right;
                Consolidate();
            }
        }
    private:
        void DecreaseKeyImpl(SharedPointer<FibonacciNode<TKey>> x){
            const auto y = x->_parent;
            if (!y && FibonacciNode<TKey>::CompareNode(x, y) < 0){
                Cut(x, y);
                CascadingCut(y);
            }
            if (FibonacciNode<TKey>::CompareNode(x, _min) < 0)
                _min = x;
        }

    public:
        void DecreaseKey(SharedPointer<FibonacciNode<TKey>> x, TKey k){
            // safe to assume that right now no nodes are being marked as minimum
            // unless the heap is operated through multiple threads, which shall be externally protected
            if (CompareKey(k, x->_key) >= 0)
                ShallThrow("key value no less than the original key value");
            x->_key = k;
            DecreaseKeyImpl(x);
        }

        void Delete(SharedPointer<FibonacciNode<TKey>> x){
            x->_minimumCausedByDeletion = true;
            DecreaseKeyImpl(x);
            x->_minimumCausedByDeletion = false;
            ExtractMin();
        }

        virtual int GetCount() const override {
            return _degree;
        }
        virtual void Add(TKey key) override { Insert(key); }
        virtual void Clear() override {
            _degree = 0;
            _min = 0;
        }
    };

    template <class TKey>
    SharedPointer<FibonacciHeap<TKey>> MakeHeapWithDefaultConstructor()
    #if ENABLECONCEPT
    requires (CharTraits<T>::Value||IsIntegral<T>::Value)
    #endif
    {
        return MakeShared<FibonacciHeap<TKey>>(GetDefaultComparator<TKey>());
    }
}
