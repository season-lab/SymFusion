import resource

MAX_VIRTUAL_MEMORY = 3 * 1024 * 1024 * 1024 
SHUTDOWN = False
RUNNING_PROCESSES = []

def setlimits():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    resource.setrlimit(resource.RLIMIT_AS,
                       (MAX_VIRTUAL_MEMORY, MAX_VIRTUAL_MEMORY))