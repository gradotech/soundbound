import numpy as np
import tensorflow as tf
import math

length = 360 # y
width = 340  # x

# Speaker coordinates
speakers = [[0, 0], [340, 0], [0, 360], [340, 360]]

step = 10

input = []
output = []

def calculate_distance(x1, y1, x2, y2):
    return math.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2)

def map_distance_to_range(distance, min_distance, max_distance):
    if min_distance == max_distance:
        if distance == min_distance:
            return 100.0
        else:
            return 0.0
    else:
        return ((distance - min_distance) / (max_distance - min_distance)) * 100

def generate_volume(x, y):
    distances = []
    for speaker in speakers:
        distance = calculate_distance(speaker[0], speaker[1], x, y)
        distances.append(distance)

    min_distance = min(distances)
    max_distance = max(distances)

    mapped_distances = []
    for distance in distances:
        mapped_distance = map_distance_to_range(distance, min_distance, max_distance)
        mapped_distances.append(int(mapped_distance))

    input.append([x, y])
    output.append(mapped_distances)

def generate_model():
    np_input = np.array(input)
    np_output = np.array(output)

    model = tf.keras.Sequential([
        tf.keras.layers.Input(shape=(2,)),
        tf.keras.layers.Dense(8, activation='relu'),
        tf.keras.layers.Dense(4)
    ])

    model.compile(optimizer='adam', loss='mean_squared_error')
    model.fit(np_input, np_output, epochs=1000, verbose=0)
    
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()

    with open('room_to_volume.tflite', 'wb') as f:
        f.write(tflite_model)

    test_input = [100, 100]
    predicted_output = model.predict(np.array([test_input]))
    print("\nTest input {}, output {}\n".format(test_input, predicted_output))

if __name__ == "__main__":
    for x in range(0, width, step):
        for y in range(0, length, step):
            generate_volume(x, y)

    generate_model()