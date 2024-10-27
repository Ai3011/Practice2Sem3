#include <string>

using namespace std;

// Узел для двусвязного списка
template <typename T>
struct NodeD {
    T data;                // Хранимые данные
    NodeD* next = nullptr;  // Указатель на следующий элемент
    NodeD* prev = nullptr;  // Указатель на предыдущий элемент
};

// Двусвязный список
template <typename T>
struct List {
    int length = 0;                  // Количество элементов в списке
    NodeD<T>* head = new NodeD<T>;    // Указатель на первый 
    NodeD<T>* tail = head;            // Указатель на последний 

    // Добавление элемента в конец списка
    void push(T data) {
        NodeD<T>* new_node = new NodeD<T>{data, nullptr, tail};  // Создаем новый узел
        tail->next = new_node;   // Связываем предыдущий элемент с новым
        tail = new_node;         // Обновляем указатель на последний элемент
        length += 1;             // Увеличиваем длину списка
    }

    // Перегрузка оператора [], возвращает элемент по индексу
    T operator[](int index) {
        return get(index);  // Используем метод get для доступа
    }

    // Удаление последнего элемента и возврат его данных
    T pop() {
        if (length == 0) {
            throw runtime_error("List is empty");  // Ошибка, если список пуст
        }

        NodeD<T>* last = tail;  // Последний узел
        T data = last->data;    // Данные последнего узла
        tail->prev->next = nullptr;  // Обнуляем указатель next у предпоследнего узла
        tail = tail->prev;           // Обновляем tail на предыдущий узел
        length--;                    // Уменьшаем длину списка

        delete last;  // Удаляем последний узел
        return data;
    }

    // Получение элемента по индексу
    T get(int index) {
        if (index >= length || index < 0) {
            throw runtime_error("Out of range");  // Ошибка, если индекс выходит за пределы
        }

        NodeD<T>* tek = head->next;  // Начинаем с первого реального узла
        while (index != 0) {
            tek = tek->next;  // Идем по списку
            index--;
        }
        return tek->data;  // Возвращаем данные
    }

    // Поиск элемента и возврат его индекса
    int find(T data) {
        for (int index = 0; index < length; index++) {
            if (get(index) == data) {
                return index;  // Возвращаем индекс найденного элемента
            }
        }
        return -1;  // Если элемент не найден
    }

    // Удаление элемента по индексу
    void del(int index) {
        if (index >= length || index < 0) {
            throw runtime_error("Out of range");  // Ошибка, если индекс некорректный
        }
        if (index == length - 1) {
            pop();  // Если удаляется последний элемент, используем pop
            return;
        }

        NodeD<T>* tek = head->next;
        while (index != 0) {
            tek = tek->next;
            index--;
        }

        NodeD<T>* pred = tek->prev;  // Предыдущий узел
        pred->next = tek->next;      // Обновляем связи
        if (tek->next) {
            tek->next->prev = pred;  // Обновляем обратную связь
        }

        delete tek;  // Удаляем узел
        length--;
    }

    // Удаление элемента по значению
    void remove(T data) {
        int index = find(data);
        if (index == -1) {
            throw runtime_error("No value");  // Ошибка, если элемента нет в списке
        }
        del(index);  // Удаляем элемент по индексу
    }

    // Вставка элемента в список по индексу
    void insert(T data, int index) {
        if (index > length || index < 0) {
            throw runtime_error("Out of range");  // Ошибка, если индекс некорректный
        }
        if (index == length) {
            push(data);  // Если индекс в конце, добавляем элемент
        } else {
            NodeD<T>* new_node = new NodeD<T>{data};
            NodeD<T>* tek = head->next;

            while (index != 0) {
                tek = tek->next;
                index--;
            }

            new_node->prev = tek->prev;
            tek->prev->next = new_node;
            tek->prev = new_node;
            new_node->next = tek;
            length++;
        }
    }

    NodeD<T>* begin(){
        return head; 
    }

    NodeD<T>* end(){
        return nullptr; 
    }

    // Очистка списка
    void clear() {
        NodeD<T>* prev;
        NodeD<T>* tek = tail;
        while (tek != head) {
            prev = tek->prev;
            delete tek;
            tek = prev;
        }
        delete tek;  // Удаляем  узел
    }
};

// Узел хеш-таблицы
template <typename T>
struct NodeH {
    string key;
    T value;
    bool state = false;  // Состояние: занято или нет
    bool deleted = false;  // Состояние, указывающий на удаление
};

// Хеш-таблица
template <typename T>
struct HashTable {
    int size = 8;
    NodeH<T>* arr = new NodeH<T>[size];  // Массив узлов хеш-таблицы

    // Деструктор хеш-таблицы
    ~HashTable() {
        delete[] arr;
    }

    // Хеш-функция для строк
    int hashFunc(string key) {
        int hash_result = 0;
        for (int i = 0; i < key.size(); i++) {
            hash_result = (31 * hash_result + key[i]) % size;
        }
        return hash_result;
    }

    // Добавление ключа и значения в таблицу
    void Add(string key, T value) {
        int index = hashFunc(key);

        while (index < size) {  //  Коллизии
            if (!arr[index].state) {
                arr[index].key = key;
                arr[index].value = value;
                arr[index].state = true;
                return;
            } else if (arr[index].key == key) {
                arr[index].value = value;  // Обновляем значение, если ключ уже существует
                return;
            }
            index++;
        }

        Resize();  // Увеличиваем размер таблицы при необходимости
        Add(key, value);  // Повторяем вставку
    }

    // Изменение размера таблицы (рехеширование)
    void Resize() {
        size *= 2;
        NodeH<T>* oldArr = arr;

        arr = new NodeH<T>[size];

        // Переносим элементы из старой таблицы в новую
        for (int i = 0; i < size / 2; i++) {
            if (oldArr[i].state && !oldArr[i].deleted) {
                Add(oldArr[i].key, oldArr[i].value);
            }
        }

        delete[] oldArr;
    }

    // Получение значения по ключу
    T Get(string key) {
        int index = hashFunc(key);

        while (index < size) {
            if (arr[index].state && arr[index].key == key) {
                return arr[index].value;
            } else if (arr[index].key == key && arr[index].deleted) {
                throw runtime_error("No such key");
            } else if (!arr[index].deleted && !arr[index].state) {
                throw runtime_error("No such key");
            }
            index++;
        }

        throw runtime_error("No such key");
    }

    // Удаление элемента по ключу
    void Remove(string key) {
        int index = hashFunc(key);
        while (index < size) {
            if (arr[index].state && arr[index].key == key) {
                if (is_same<T, List<string>>::value) {
                    arr[index].value.clear();  // Очистка списка, если это список
                }
                arr[index].state = false;
                arr[index].deleted = true;
                return;
            } else if (arr[index].key == key && arr[index].deleted) {
                throw runtime_error("No such key");
            } else if (!arr[index].deleted && !arr[index].state) {
                throw runtime_error("No such key");
            }
            index++;
        }
        throw runtime_error("No such key");
    }
};
