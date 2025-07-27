import {Typography, Button, Slider, ConfigProvider, notification} from "antd";
import {useEffect, useState} from "react";

const {Title} = Typography

type State = {
    running: boolean
    freeHeap: number
    shooter: {
        top_speed: number
        bottom_speed: number
    }
    delivery: {
        speed: number
    }
}

// работает только с теннисной пушкой в локальной сети
const url = 'http://tennis-gun.local'

function App() {
    const [notify, notificationContext] = notification.useNotification();
    const [state, setState] = useState<State | undefined>(undefined)
    console.log('state', state)
    // const [error, setError] = useState<string | undefined>(undefined)
    const [pendingRun, setPendingRun] = useState(false)
    const [shooterBottomSpeed, setShooterBottomSpeed] = useState(0)
    const [shooterTopSpeed, setShooterTopSpeed] = useState(0)
    const [deliverySpeed, setDeliverySpeed] = useState(0)

    useEffect(() => {
        setShooterTopSpeed(state?.shooter.top_speed ?? 0)
    }, [state?.shooter.top_speed]);
    useEffect(() => {
        setShooterBottomSpeed(state?.shooter.bottom_speed ?? 0)
    }, [state?.shooter.bottom_speed]);
    useEffect(() => {
        setDeliverySpeed(state?.delivery.speed ?? 0)
    }, [state?.delivery.speed]);

    useEffect(() => {
        const source = new EventSource(url + '/status-events')
        source.onerror = evt => {
            console.error(evt)
            notify.error({message: 'status-events error'})
        }
        source.onmessage = evt => {
            setState(JSON.parse(evt.data))
        }
        return () => {
            source.close()
        }
    }, []);

    const onStart = async () => {
        if (pendingRun) return
        setPendingRun(true)
        try {
            await fetch(url + '/start', {method: 'POST'})
        } catch (e) {
            console.error(e)
            // @ts-ignore
            notify.error({message: e.message})
        } finally {
            setPendingRun(false)
        }
    }

    const onStop = async () => {
        if (pendingRun) return
        setPendingRun(true)
        try {
            await fetch(url + '/stop', {method: 'POST'})
        } catch (e) {
            console.error(e)
            // @ts-ignore
            notify.error({message: e.message})
        } finally {
            setPendingRun(false)
        }
    }

    const onShooterTopSpeedChange = async (value: number) => {
        await fetch(url + `/shooter?top_speed=${value}`, {method: 'PATCH',})
    }
    const onShooterBottomSpeedChange = async (value: number) => {
        await fetch(url + `/shooter?bottom_speed=${value}`, {method: 'PATCH',})
    }

    const onDeliverySpeedChange = async (value: number) => {
        await fetch(url + `/delivery?speed=${value}`, {method: 'PATCH',})
    }

    return (
        <main className={'h-dvh w-full flex flex-col items-center justify-center p-4 md:p-10'}>
            {notificationContext}
            <div>
                {
                    state ? (
                        <Button style={{padding: '32px 64px'}} size={'large'} variant={"solid"}
                                color={!state.running ? 'primary' : 'danger'}
                                onClick={state.running ? onStop : onStart}>
                            {state.running ? 'Stop' : 'Start'}
                        </Button>
                    ) : (
                        <Button style={{padding: '32px 64px'}} size={'large'} variant={"solid"} color={'default'}
                                disabled={true}>
                            Unknown
                        </Button>
                    )
                }
            </div>
            <ConfigProvider
                theme={{
                    components: {
                        Slider: {
                            handleSize: 32,
                            handleSizeHover: 48,
                            railSize: 48,
                        },
                    },
                }}
            >
                <div className={'mt-8 mb-12 flex flex-row gap-16 items-center justify-center h-[300px]'}>
                    <div className={'h-full'}>
                        <Title level={3}>
                            Shooter Top <span className={'inline-block min-w-[64px]'}>{shooterTopSpeed}%</span>
                        </Title>
                        <Slider min={0} max={100} vertical value={shooterTopSpeed}
                                onChangeComplete={v => onShooterTopSpeedChange(v)}
                                onChange={v => setShooterTopSpeed(v)}
                        />
                    </div>
                    <div className={'h-full'}>
                        <Title level={3}>
                            Shooter Bottom <span className={'inline-block min-w-[64px]'}>{shooterBottomSpeed}%</span>
                        </Title>
                        <Slider min={0} max={100} vertical value={shooterBottomSpeed}
                                onChangeComplete={v => onShooterBottomSpeedChange(v)}
                                onChange={v => setShooterBottomSpeed(v)}
                        />
                    </div>
                    <div className={'h-full'}>
                        <Title level={3}>
                            Delivery <span className={'inline-block min-w-[64px]'}>{deliverySpeed}%</span>
                        </Title>
                        <Slider min={0} max={100} vertical value={deliverySpeed}
                                onChangeComplete={v => onDeliverySpeedChange(v)}
                                onChange={v => setDeliverySpeed(v)}
                        />
                    </div>
                </div>
            </ConfigProvider>
            <div className={'mt-4'}>
                <Title level={3}>
                    Free heap space: {state?.freeHeap ?? 0}
                </Title>
            </div>
        </main>
    )
}

export default App
