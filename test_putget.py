from multiprocessing import Process, Queue

q = Queue()

iterations = 10000000

def produce(q):
    for i in range(iterations):
        q.put(i)
    
if __name__ ==  "__main__":
    t = Process(target=produce, args=(q,))
    t.start()

    previous = -1
    for i in range(iterations):
        m = q.get()
        if m != previous + 1:
            print "Fail at:", previous, m
            break
        previous = m

    print "done"
