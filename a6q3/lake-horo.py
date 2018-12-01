#!/bin/python

###################################################
# Group Details:
# asrivas3 Abhishek Kumar Srivastava
# ajain28 Abhash Jain
#
# CSC-548 Assignment#6 Problem#3
##################################################

#Import libraries for simulation
import tensorflow as tf
import numpy as np
import sys
import time
import horovod.tensorflow as hvd
import fcntl

#Imports for visualization
import PIL.Image

# Initialized Horovod
hvd.init()

def DisplayArray(a, fmt='jpeg', rng=[0,1]):
  """Display an array as a picture."""
  a = (a - rng[0])/float(rng[1] - rng[0])*255
  a = np.uint8(np.clip(a, 0, 255))
  if hvd.rank() == 0: 
        a = a[0:-3]
  else:
        a = a[3:]
  with open("lake_py_" + str(hvd.rank()) + ".jpg", "w") as f:
      PIL.Image.fromarray(a).save(f, "jpeg")

sess = tf.InteractiveSession()

# Function to print the dat files for the images
def printHeatmap(u,n,h):
    f = open("lake_c_"+str(hvd.rank())+".dat","w")
    if hvd.rank() != 0:
        u = u[3:]
    else:
        u = u[0:-3]
    for i in range(0,n):
        for j in range(0,n):
            f.write(str(float(i)*h)+" "+str(float(j)*h)+" "+str(u[i,j])+"\n")
    f.close()

# Computational Convenience Functions
def make_kernel(a):
  """Transform a 2D array into a convolution kernel"""
  a = np.asarray(a)
  a = a.reshape(list(a.shape) + [1,1])
  return tf.constant(a, dtype=1)

def simple_conv(x, k):
  """A simplified 2D convolution operation"""
  x = tf.expand_dims(tf.expand_dims(x, 0), -1)
  y = tf.nn.depthwise_conv2d(x, k, [1, 1, 1, 1], padding='SAME')
  return y[0, :, :, 0]

def laplace(x):
  """Compute the 2D laplacian of an array"""
#  5 point stencil #
  five_point = [[0.0, 1.0, 0.0],
                [1.0, -4., 1.0],
                [0.0, 1.0, 0.0]]

#  9 point stencil #
  nine_point = [[0.25, 1.0, 0.25],
                [1.00, -5., 1.00],
                [0.25, 1.0, 0.25]]
	
#  13 point stencil #
  thirteen_point = [[0.0,0.0,0.0,0.125,0.0,0.0,0.0],
                     [0.0,0.0,0.0,0.25,0.0,0.0,0.0],
                     [0.0,0.0,0.0,1.0,0.0,0.0,0.0],
                     [0.125,0.25,1.0,-5.5,1.0,0.25,0.125],
                     [0.0,0.0,0.0,1.0,0.0,0.0,0.0],
                     [0.0,0.0,0.0,0.25,0.0,0.0,0.0],
                     [0.0,0.0,0.0,0.125,0.0,0.0,0.0]]

  #laplace_k = make_kernel(nine_point)
  laplace_k = make_kernel(thirteen_point)
  return simple_conv(x, laplace_k)

# Define the PDE
if len(sys.argv) != 4:
	print "Usage:", sys.argv[0], "N npebs num_iter"
	sys.exit()
	
N = int(sys.argv[1])
npebs = int(sys.argv[2])
num_iter = int(sys.argv[3])

# Initial Conditions -- some rain drops hit a pond

# Set everything to zero
# allocated extra space for transfer data
u_init  = np.zeros([N+3, N], dtype=np.float32)
ut_init = np.zeros([N+3, N], dtype=np.float32)

# Some rain drops hit a pond at random points
if hvd.rank() == 0:
# if rank is 0 then random points for both row and column
# are chosen from 0 to N.
    for n in range(npebs):
        a,b = np.random.randint(0, N, 2)
        u_init[a,b] = np.random.uniform()
else:
# if rank is not 0 then random points for row value is chosen from 3 and N+3 
# and for column the value is chosen from 0 to N.
    for n in range(npebs):
        a = np.random.randint(3,N+3)
        b = np.random.randint(0,N)
        u_init[a,b] = np.random.uniform()

# Parameters:
# eps -- time resolution
# damping -- wave damping
eps = tf.placeholder(tf.float32, shape=())
damping = tf.placeholder(tf.float32, shape=())

# Create variables for simulation state
U  = tf.Variable(u_init)
Ut = tf.Variable(ut_init)

# Discretized PDE update rules
U_ = U + eps * Ut
Ut_ = Ut + eps * (laplace(U) - damping * Ut)

# Operation to update the state
# rank 0 will broadcast last 3 rows of its lake
# rank 1 will broadcast first 3 rows of its lake
step = tf.group(
  U.assign(U_),
  Ut.assign(Ut_),
  tf.assign(U[:3],hvd.broadcast(U[-6:-3],0)),
  tf.assign(Ut[:3],hvd.broadcast(Ut[-6:-3],0)),
  tf.assign(U[-3:],hvd.broadcast(U[3:6],1)),
  tf.assign(Ut[-3:],hvd.broadcast(Ut[3:6],1)))

# Initialize state to initial conditions
tf.global_variables_initializer().run()

# Run num_iter steps of PDE
start = time.time()
for i in range(num_iter):
  # Step simulation
  step.run({eps: 0.06, damping: 0.03})

end = time.time()
print('Elapsed time: {} seconds'.format(end - start))  
#DisplayArray(U.eval(), rng=[-0.1, 0.1])
h = (1.0 - 0.0)/N
printHeatmap(U.eval(), N, h)
