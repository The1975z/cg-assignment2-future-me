# cg-assignment2-future-me

งาน Assignment 2 วิชา Computer Graphics (05506014) หัวข้อ **FUTURE ME**

Future me ของผมคืออยากเป็นนักบิน เลยทำฉากเครื่อง Boeing 747-8 บินเหนือทะเลเมฆตอนพระอาทิตย์ขึ้น แล้วเขียนควันบนฟ้าว่า FUTURE ME / BORN TO FLY.

![main view](docs/preview.png)

มุมอื่นในโปรแกรม (กด 2, 3, 4)

![chase view](docs/chase.png)

![window view](docs/window.png)

![cockpit view](docs/cockpit.png)

## ในฉากมีอะไรบ้าง

- Modern OpenGL 3.3 + Phong lighting (ambient / diffuse / specular) ส่วนไฟปลายปีก strobe beacon และไฟในห้องนักบินเป็น point light
- Shadow mapping ตาม Lab 13 เครื่องบินกับไอเครื่องยนต์มีเงาตกบนเมฆ ใช้ PCF ให้ขอบเงานุ่ม
- God rays ตรงพระอาทิตย์ที่อยู่หลังเมฆ และ bloom
- ท้องฟ้า ทะเลเมฆ เมฆก้อน ภูเขา สร้างจาก vertex กับ noise เอง มีแค่ตัวเครื่องบินที่เป็นโมเดล
- กล้องแบบ Lab 12 (yaw / pitch + WASD) และมีมุมกล้องให้เลือก

## ปุ่ม

- `1` มุมหลัก
- `2` มุมตามหลังเครื่อง
- `3` มองจากหน้าต่างผู้โดยสาร
- `4` มองจากที่นั่งกัปตัน
- `C` เปิด/ปิดกล้องอิสระ (เมาส์หันมอง, WASD เดิน, Q/E ลง/ขึ้น, Shift เร็วขึ้น)
- `R` กลับมุมหลัก
- `P` เซฟภาพ 800x600
- `Esc` ออก

## วิธีรัน

ใช้ Windows + Visual Studio 2026 + CMake + vcpkg ตั้งค่าแบบเดียวกับ [OpenGLStarter](https://github.com/momotheoxy/OpenGLStarter) ของวิชา

```
cmake --preset win-msvc-2026
cmake --build --preset build-win-2026-release
.\build\win-msvc-2026\Release\OpenGLStarter.exe
```

ใน VS Code กด F7 build แล้ว F5 รัน อย่ากดปุ่ม Run C/C++ File เพราะมันจะ compile ด้วย g++ แล้วหา GLEW ไม่เจอ

ถ้าอยากได้ภาพเลยใช้ `OpenGLStarter.exe --capture shot.png` ต่อท้ายด้วยเลข 1-4 ถ้าอยากได้มุมอื่น

## หมายเหตุ

โมเดล 747 กับ texture ไม่ได้อัปขึ้นมาเพราะเป็นของ FAIB ถ้าจะรันต้องใส่ไฟล์เอง

- `Models/Boeing 747-8I_textured.obj` และ `Models/Boeing 747-8I_textured.mtl`
- `Textures/faib_b7478_t_0.jpeg`

## เครดิต

- Starter project: [OpenGLStarter](https://github.com/momotheoxy/OpenGLStarter)
- Shadow mapping: Lab 13 ของวิชา และ [LearnOpenGL](https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping)
- God rays: GPU Gems 3 บทที่ 13 Volumetric Light Scattering as a Post-Process
- ACES tone mapping: Krzysztof Narkowicz
- Noise hash: Inigo Quilez
- โมเดล Boeing 747-8I และ texture ลาย Sunrise: FAIB
