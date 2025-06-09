import rclpy
from rclpy.node import Node
from std_msgs.msg import String
from geometry_msgs.msg import Vector3
from skyfield.api import load, Topos, N, E
from datetime import datetime

class PlanetLocatorNode(Node):
    def __init__(self):
        super().__init__('planet_locator_node')
        self.subscription = self.create_subscription(
            String,
            '/planet_info',
            self.listener_callback,
            10
        )
        self.publisher = self.create_publisher(
            Vector3,
            '/planet_direction',
            10
        )
        
        # Загрузка данных Skyfield
        self.planets = load('de421.bsp')
        self.ts = load.timescale()
        
        # Координаты места наблюдения (Москва, МИРЭА)
        self.latitude = 55.9154  # Просто число!
        self.longitude = 37.3694  # Просто число!
        
        # Карта номеров планет
        self.planet_number_map = {
            '1': 'sun',
            '2': 'mercury',
            '3': 'venus',
            '4': 'mars',
            '5': 'jupiter barycenter',
            '6': 'saturn barycenter',
            '7': 'uranus barycenter',
            '8': 'neptune barycenter'
        }

    def listener_callback(self, msg):
        try:
            planet_number, time_str = msg.data.split('|')
            planet_name = self.planet_number_map.get(planet_number.strip())
            
            if planet_name is None:
                self.get_logger().error(f'Некорректный номер планеты: {planet_number}')
                return
                
            current_time = datetime.strptime(time_str.strip(), '%Y-%m-%d %H:%M:%S')
            altitude, azimuth, distance = self.compute_planet_direction(
                planet_name, 
                current_time
            )
            
            direction_msg = Vector3()
            direction_msg.x = float(azimuth)       # Азимут
            direction_msg.y = float(altitude)       # Высота
            direction_msg.z = float(distance)       # Дистанция
            
            self.publisher.publish(direction_msg)
            
            self.get_logger().info(
                f'Планета: {planet_name.title()} (№{planet_number}) | '
                f'Азимут: {azimuth:.2f}°, Высота: {altitude:.2f}°, '
                f'Дистанция: {distance:.2f} км'
            )
            
        except Exception as e:
            self.get_logger().error(f'Ошибка обработки сообщения: {e}')

    def compute_planet_direction(self, planet_name, current_time):
        # Корректное создание временной метки
        time = self.ts.utc(
            current_time.year,
            current_time.month,
            current_time.day,
            current_time.hour,
            current_time.minute,
            current_time.second
        )
        
        # Корректное создание точки наблюдения
        location = Topos(
            latitude_degrees=self.latitude,
            longitude_degrees=self.longitude
        )
        earth = self.planets['earth']
        observer = earth + location
        
        # Получаем положение планеты
        planet = self.planets[planet_name]
        
        # Вычисляем относительное положение
        apparent = observer.at(time).observe(planet).apparent()
        
        # Преобразуем в горизонтальные координаты
        altitude, azimuth, distance = apparent.altaz()
        
        # Возвращаем числовые значения с округлением
        return (
            round(altitude.degrees, 2),
            round(azimuth.degrees, 2),
            round(distance.km, 2)
        )

def main(args=None):
    rclpy.init(args=args)
    node = PlanetLocatorNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()