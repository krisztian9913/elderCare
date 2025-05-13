import folium
from geopy.geocoders import Nominatim

geolocator = Nominatim(user_agent="my_geopy_app")

def getLocation(coordinates):
    latitude = str(coordinates[0])
    longitude = str(coordinates[1])

    location = geolocator.reverse(latitude + "," + longitude)

    #print(location)

    address = location.raw['address']
    #print(address)

    city = address.get('city', '')
    road = address.get('road', '')

    return city, road



path = [
    [46.200785, 20.107884],
    [46.274414, 20.097040],
    [46.274560, 20.096652],
    [46.421521, 19.954551],
    [46.765755, 19.765679],
    [47.048673, 19.561351],
    [47.315444, 19.263522],
    [47.451753, 19.119646],
    [47.494584, 19.108996],
]

pathTime = [
    "2025-05-05 06:29:31",
    "2025-05-05 06:53:55",
    "2025-05-05 07:14:02",
    "2025-05-05 07:36:53",
    "2025-05-05 07:57:06",
    "2025-05-05 08:18:13",
    "2025-05-05 08:41:01",
    "2025-05-05 09:02:27",
    "2025-05-05 09:23:34",

]

path2 = [
    [47.508727, 19.101498],
    [47.508959, 19.102665],
    [47.446556, 19.125224],
    [47.194968, 19.424709],
    [46.888275, 19.626178],
    [46.573112, 19.873961],
    [46.274188, 20.097423],
    [46.244122, 20.121344],
    [46.218357, 20.110647],
]

path2Time = [
    "2025-05-05 11:49:19",
    "2025-05-05 12:11:58",
    "2025-05-05 12:33:06",
    "2025-05-05 12:53:18",
    "2025-05-05 13:14:24",
    "2025-05-05 13:35:44",
    "2025-05-05 16:42:31",
    "2025-05-05 17:04:21",
    "2025-05-05 17:24:34",
]



m = folium.Map(location=path[0], zoom_start=13)

folium.PolyLine(path, color="blue", weight=2.5, opacity=1).add_to(m)
folium.PolyLine(path2, color="red", weight=2.5, opacity=1).add_to(m)


for i in range(len(path)):

    city, road = getLocation(path[i])

    street = pathTime[i] + "\n" + city + "\n" + road
    folium.Marker(path[i], popup=street).add_to(m)

for i in range(len(path2)):

    city, road = getLocation(path2[i])

    street = path2Time[i] + "\n" + city + "\n" + road
    folium.Marker(path2[i], popup=street).add_to(m)




m.save("utvonal0505.html")