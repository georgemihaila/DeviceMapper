using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace DeviceMapper.Backend.Infra
{
    public class CircularList<T> : IList<T>
    {
        private readonly int _capacity;
        private readonly List<T> _list;
        private int _currentIndex = 0;
        public CircularList(int capacity)
        {
            _capacity = capacity;
            _list = new List<T>(capacity);
        }

        public T this[int index] { get => ((IList<T>)_list)[index]; set => ((IList<T>)_list)[index] = value; }

        public int Count => ((ICollection<T>)_list).Count;

        public bool IsReadOnly => ((ICollection<T>)_list).IsReadOnly;

        public void Add(T element)
        {
            _list.Insert(_currentIndex++, element);
            if (_list.Count > _currentIndex)
            {
                _list.RemoveAt(_currentIndex);
            }
            if (_currentIndex == _capacity)
            {
                _currentIndex = 0;
            }
        }

        public void Clear()
        {
            ((ICollection<T>)_list).Clear();
        }

        public bool Contains(T item)
        {
            return ((ICollection<T>)_list).Contains(item);
        }

        public void CopyTo(T[] array, int arrayIndex)
        {
            ((ICollection<T>)_list).CopyTo(array, arrayIndex);
        }

        public IEnumerator<T> GetEnumerator()
        {
            return ((IEnumerable<T>)_list).GetEnumerator();
        }

        public int IndexOf(T item)
        {
            return ((IList<T>)_list).IndexOf(item);
        }

        public void Insert(int index, T item)
        {
            ((IList<T>)_list).Insert(index, item);
        }

        public bool Remove(T item)
        {
            return ((ICollection<T>)_list).Remove(item);
        }

        public void RemoveAt(int index)
        {
            ((IList<T>)_list).RemoveAt(index);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return ((IEnumerable)_list).GetEnumerator();
        }
    }
}
