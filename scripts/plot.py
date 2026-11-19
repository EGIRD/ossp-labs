import matplotlib.pyplot as plt

runs_A = [9130, 9267, 9356]   
runs_B = [15063, 15160, 15097]

reads = [150, 104]
writes = [100, 250]

configs = ['3 Чит / 2 Пис', '3 Чит / 5 Пис']
avg_times = [sum(runs_A)/len(runs_A), sum(runs_B)/len(runs_B)]

plt.figure(figsize=(8, 5))
bars = plt.bar(configs, avg_times, color=['#4C72B0', '#DD8452'])
plt.title('Среднее время выполнения программы (Execution Time)')
plt.xlabel('Конфигурация (Читатели / Писатели)')
plt.ylabel('Время, мс')
plt.grid(axis='y', linestyle='--', alpha=0.7)

for bar in bars:
    yval = bar.get_height()
    plt.text(bar.get_x() + bar.get_width()/2, yval + 100, round(yval, 1), ha='center', va='bottom')

plt.savefig('report/graph1_time.png', dpi=300)
plt.show()

plt.figure(figsize=(8, 5))
x = range(len(configs))
width = 0.35

plt.bar([i - width/2 for i in x], reads, width, label='Успешные чтения', color='#55A868')
plt.bar([i + width/2 for i in x], writes, width, label='Успешные записи', color='#C44E52')

plt.title('Количество успешных операций до завершения программы')
plt.xlabel('Конфигурация')
plt.ylabel('Количество операций')
plt.xticks(x, configs)
plt.legend()
plt.grid(axis='y', linestyle='--', alpha=0.7)

plt.savefig('report/graph2_ops.png', dpi=300)
plt.show()

print("Графики успешно сохранены в папку report/")