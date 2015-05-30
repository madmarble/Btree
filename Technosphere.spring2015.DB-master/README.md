# Структура репозитория:
```
├── Makefile
├── README.md
├── gen_workload
│   ├── data									 - Каталог с данными для тестовой системы
│   │   ├── keys.txt
│   │   └── values.txt
│   ├── gen_workload.py				 - Скрипт для генерации Workload'a
│   └── lib
│  			├── __init__.py
│  			├── database.py
│  			└── ordered_set.py
├── libmydb.so
├── mydb.c									 - Базовая реализация
├── mydb.h									 - Базовый хедер
├── runner
│   ├── Makefile						 - Makefile для собрки и тестирования программы загрузки
│   ├── README
│   ├── database.cpp
│   ├── database.h
│   ├── main.cpp
│   └── test.sh
├── sophia										- Папка с примером (с использованием библиотеки sophia)
│   ├── Makefile							- Makefile для сборки проекта
│   ├── README.rst
│   ├── db_sphia.c						- Пример реализации
│   ├── db_sphia.h						- Копия mydb.h с небольшими изменениями
│   └── third_party						- Submodule с репозитоорием библиотеки sophia
└── workloads									- Примеры workload'ов
		├── workload.lat.in
		├── workload.lat.out
		├── workload.old.in
		├── workload.old.out
		├── workload.uni.in
		└── workload.uni.out
```

Для того, чтобы собрать `sophia` нужно скачать репозиторий с помощью
`git clone git://github.com/bigbes/Technosphere.spring2015.DB.git --recursive`
или в уже скачанном репозитории сделать `git submodule update --init`


# Тестирующая система

Для использование необходимо поставить PyYAML:

* `pip install pyyaml`
* `sudo apt-get install python-yaml` # на Ubuntu/Debian

Она состоит из двух файлов:

* `gen_workload/gen_workload.py`
* `runner/test_speed`

## gen_workload/gen_workload.py

Первый из них отвечает за создание файлов для тестирования, а второй за запуск их на вашем хранилище.

### Конфигурационный файл

В первый файл вы можете передать конфигурационный файл, например:

```
---
get: 20
put: 80
del: 80
shuffle: True
distrib: "uniform"
ops: 10000
...
```

* get + put + del = 100 - процентное отношение обеих операций (int)
* shuffle, отвечает за то, чтобы операции put/get/del были перемешаны (иначе, сначала будут идти все put, а затем все get, затем del) (True/False)
* distrib - тип распределения ("uniform"/"latest"/"oldest"/"none")
* ops - итоговое кол-во операций (int)

### Аргументы командной строки

Передавать конфигурационный файл можно с помощью `--config`
Указывать имя файла нагрузки можно с помощью `--output`

### Пример

Находясь в папке example:
```
> python ../gen_workload/gen_workload.py --output workload
--------------------------------------------------------------------------------
Workload successfully generated
Output: /home/bigbes/src/hw1/example/workload.in
--------------------------------------------------------------------------------
> python ../gen_workload/gen_workload.py --output workload --config example.schema.yml
--------------------------------------------------------------------------------
Workload successfully generated
Output: /home/bigbes/src/hw1/example/workload.in
Config: /home/bigbes/src/hw1/example/example.schema.yml
--------------------------------------------------------------------------------
```

# Ограничения

Для того, чтобы ваша библиотека смогла загрузиться нужны следующие глобальные методы:
```
struct DB *dbcreate(const char *path, const struct DBC conf);

int db_close(struct DB *db);
int db_del(const struct DB *, void *, size_t);
int db_get(const struct DB *, void *, size_t, void **, size_t *);
int db_put(const struct DB *, void *, size_t, void * , size_t  );
```

Данная библиотека (`runner/database.cpp`) использует dlopen, для загрузки DLL и импорта функций из неё.

О том как запускать и тестировать ваше дерево - пройдите в папку `runner`
